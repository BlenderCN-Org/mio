#include "mio.h"
#include <ctype.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

extern lua_State *L; /* in bind.c */

void warn(char *fmt, ...)
{
	va_list ap;
	char buf[256];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s\n", buf);
	console_printnl(buf);
}

#define PS1 "> "
#define PS2 ">>"
#define COLS 80
#define ROWS 23
#define LAST (ROWS-1)
#define INPUT 80-2
#define HISTORY 100
#define TABSTOP 8

static char screen[ROWS][COLS+1] = {{0}};
static int tail = 0;
static char input[INPUT] = {0};
static int cursor = 0;
static int end = 0;

static struct history_entry {
	TAILQ_ENTRY(history_entry) list;
	char s[INPUT];
} *hist_look;

static TAILQ_HEAD(history_list, history_entry) history;

static void scrollup(void)
{
	int i;
	for (i = 1; i < ROWS; i++)
		memcpy(screen[i-1], screen[i], COLS);
	screen[LAST][0] = 0;
	tail = 0;
}

void console_putc(int c)
{
	if (tail >= COLS) {
		scrollup();
		screen[LAST][tail++] = c;
	} else if (c == '\n') {
		scrollup();
	} else if (c == '\t') {
		int n = ((tail+8)/8)*8 - tail;
		while (n--)
			console_putc(' ');
	} else {
		screen[LAST][tail++] = c;
	}
	screen[LAST][tail] = 0;
}

void console_print(const char *s)
{
	while (*s) console_putc(*s++);
}

void console_printnl(const char *s)
{
	while (*s) console_putc(*s++);
	scrollup();
}

static int ffi_print(lua_State *L)
{
	int i, n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (i=1; i<=n; i++) {
		const char *s;
		lua_pushvalue(L, -1); /* tostring */
		lua_pushvalue(L, i); /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1); /* get result */
		if (!s) return luaL_error(L, "'tostring' must return a string to 'print'");
		if (i > 1) console_print(" ");
		console_print(s);
		lua_pop(L, 1); /* pop result */
	}
	console_printnl("");
	return 0;
}

void console_init(void)
{
	if (!L) {
		L = luaL_newstate();
		luaL_openlibs(L);
	}

	lua_register(L, "print", ffi_print);
}

static void console_insert(int c)
{
	hist_look = NULL;
	if (end + 1 < INPUT) {
		memmove(input + cursor + 1, input + cursor, end - cursor);
		input[cursor++] = c;
		input[++end] = 0;
	}
}

static void console_delete(void)
{
	hist_look = NULL;
	if (cursor > 0) {
		memmove(input + cursor - 1, input + cursor, end - cursor);
		input[--end] = 0;
		--cursor;
	}
}

static void console_history_push(char *s)
{
	if (strlen(s) > 0) {
		struct history_entry *line = malloc(sizeof *line);
		strlcpy(line->s, s, sizeof line->s);
		TAILQ_INSERT_HEAD(&history, line, list);
		hist_look = NULL;
	}
}

static void console_history_prev(void)
{
	struct history_entry *candidate;
	candidate = hist_look ? TAILQ_NEXT(hist_look, list) : TAILQ_FIRST(&history);
	if (candidate) {
		hist_look = candidate;
		memcpy(input, hist_look->s, INPUT);
		cursor = end = strlen(input);
	}
}

static void console_history_next(void)
{
	hist_look = hist_look ? TAILQ_PREV(hist_look, history_list, list) : NULL;
	if (hist_look) {
		memcpy(input, hist_look->s, INPUT);
		cursor = end = strlen(input);
	} else {
		cursor = end = 0;
		input[0] = 0;
	}
}

static void console_enter(void)
{
	char cmd[INPUT];

	console_history_push(input);
	console_print(PS1);
	console_printnl(input);

	if (input[0] == '=') {
		strlcpy(cmd, "return ", sizeof cmd);
		strlcat(cmd, input + 1, sizeof cmd);
	} else {
		strlcpy(cmd, input, sizeof cmd);
	}

	int status;
	status = luaL_dostring(L, cmd);
	if (status && !lua_isnil(L, -1)) {
		const char *msg = lua_tostring(L, -1);
		if (msg == NULL) msg = "(error object is not a string)";
		console_printnl(msg);
		lua_pop(L, 1);
	}
	if (lua_gettop(L) > 0) {
		lua_getglobal(L, "print");
		lua_insert(L, 1);
		lua_pcall(L, lua_gettop(L)-1, 0, 0);
	}

	input[0] = 0;
	cursor = end = 0;
}

#define CTL(x) (x-64)

void console_keyboard(int key, int mod)
{
	if (key >= 0xE000 && key < 0xF900) return; // in PUA
	if (key >= 0x10000) return; // outside BMP

	if (mod & GLUT_ACTIVE_ALT) return;

	if (mod & GLUT_ACTIVE_CTRL) {
		switch (key) {
		case CTL('A'): cursor = 0; break;
		case CTL('E'): cursor = end; break;
		case CTL('U'):
			while (cursor > 0) console_delete();
			break;
		case CTL('W'):
			while (cursor > 0 && !isalnum(input[cursor-1])) console_delete();
			while (cursor > 0 && isalnum(input[cursor-1])) console_delete();
			break;
		}
		return;
	}

	if (key == '\r') {
		console_enter();
	} else if (key == 0x08 || key == 0x7F) {
		console_delete();
	} else if (isprint(key)) {
		console_insert(key);
	}
}

void console_special(int key, int mod)
{
	switch (key) {
	case GLUT_KEY_LEFT: if (cursor > 0) cursor--; break;
	case GLUT_KEY_RIGHT: if (cursor < end) cursor++; break;
	case GLUT_KEY_HOME: cursor = 0; break;
	case GLUT_KEY_END: cursor = end; break;
	case GLUT_KEY_UP: console_history_prev(); break;
	case GLUT_KEY_DOWN: console_history_next(); break;
	}
}

void console_draw(mat4 clip_from_view, mat4 view_from_world, struct font *font, float size)
{
	float model_view[16];
	float screenw = 2 / clip_from_view[0]; // assume we have an orthogonal matrix
	float cellw = font_width(font, size, "0");
	float cellh = size + 2;
	float ascent = size * 0.8;
	float descent = size * 0.2;
	float margin = 8;

	int x0 = (screenw - cellw * COLS) / 2;
	int y0 = margin;
	int x1 = x0 + cellw * COLS;
	int y1 = y0 + cellh * ROWS;
	float x, y;
	int i;

	mat_identity(model_view);

	draw_begin(clip_from_view, view_from_world);
	draw_set_color(0, 0, 0, 0.8);
	draw_rect(x0-margin, y0-margin, x1+margin, y1+margin);
	draw_end();

	text_begin(clip_from_view, view_from_world);
	text_set_color(1, 1, 1, 1);
	text_set_font(font, size);

	y = y0 + ascent;

	for (i = 0; i < ROWS; i++) {
		x = text_show(x0, y, screen[i]);
		y += cellh;
	}
	y -= cellh;
	x = text_show(x, y, PS1);
	x = text_show(x, y, input);
	x -= text_width(input + cursor);

	text_end();

	draw_begin(clip_from_view, view_from_world);
	draw_set_color(1, 1, 1, 1);
	draw_line(x+1, y-ascent, 0, x+1, y+descent-1, 0);
	draw_end();
}
