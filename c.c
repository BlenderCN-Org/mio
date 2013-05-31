#include "mio.h"

extern struct scene *scene;

static int screenw = 800, screenh = 600;
static int mousex, mousey, mouseleft = 0, mousemiddle = 0, mouseright = 0;
static int lasttime = 0;

static int showconsole = 0;

static struct font *droid_sans;
static struct font *droid_sans_mono;

static float cam_dist = 5;
static float cam_yaw = 0;
static float cam_pitch = -20;

void togglefullscreen(void)
{
	static int oldw = 100, oldh = 100, oldx = 0, oldy = 0;
	static int isfullscreen = 0;
	if (!isfullscreen) {
		oldw = glutGet(GLUT_WINDOW_WIDTH);
		oldh = glutGet(GLUT_WINDOW_HEIGHT);
		oldx = glutGet(GLUT_WINDOW_X);
		oldy = glutGet(GLUT_WINDOW_Y);
		glutFullScreen();
	} else {
		glutPositionWindow(oldx, oldy);
		glutReshapeWindow(oldw, oldh);
	}
	isfullscreen = !isfullscreen;
}

static void mouse(int button, int state, int x, int y)
{
	state = state == GLUT_DOWN;
	if (button == GLUT_LEFT_BUTTON) mouseleft = state;
	if (button == GLUT_MIDDLE_BUTTON) mousemiddle = state;
	if (button == GLUT_RIGHT_BUTTON) mouseright = state;
	mousex = x;
	mousey = y;
	glutPostRedisplay();
}

static void motion(int x, int y)
{
	int dx = x - mousex;
	int dy = y - mousey;
	if (mouseleft) {
		cam_yaw -= dx * 0.3;
		cam_pitch -= dy * 0.2;
		if (cam_pitch < -85) cam_pitch = -85;
		if (cam_pitch > 85) cam_pitch = 85;
		if (cam_yaw < 0) cam_yaw += 360;
		if (cam_yaw > 360) cam_yaw -= 360;
	}
	if (mousemiddle || mouseright) {
		cam_dist += dy * 0.01 * cam_dist;
		if (cam_dist < 0.1) cam_dist = 0.1;
		if (cam_dist > 100) cam_dist = 100;
	}
	mousex = x;
	mousey = y;
	glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y)
{
	int mod = glutGetModifiers();
	if ((mod & GLUT_ACTIVE_ALT) && key == '\r')
		togglefullscreen();
	else if (key ==	'`')
		showconsole = !showconsole;
	else if (showconsole)
		console_keyboard(key, mod);
	else switch (key) {
		case 27: case 'q': exit(0); break;
		case 'f': togglefullscreen(); break;
	}

	lasttime = glutGet(GLUT_ELAPSED_TIME);

	glutPostRedisplay();
}

static void special(int key, int x, int y)
{
	int mod = glutGetModifiers();
	if (key == GLUT_KEY_F4 && mod == GLUT_ACTIVE_ALT)
		exit(0);
	if (showconsole)
		console_special(key, mod);
	glutPostRedisplay();
}

static void reshape(int w, int h)
{
	screenw = w;
	screenh = h;
	glViewport(0, 0, w, h);
}

static void display(void)
{
	float projection[16];
	float view[16];

	int thistime, timediff;

	thistime = glutGet(GLUT_ELAPSED_TIME);
	timediff = thistime - lasttime;
	lasttime = thistime;

	glViewport(0, 0, screenw, screenh);

	glClearColor(0.05, 0.05, 0.05, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat_perspective(projection, 75, (float)screenw / screenh, 0.1, 1000);
	mat_identity(view);
	mat_rotate_x(view, -90);

	mat_translate(view, 0, cam_dist, -cam_dist / 5);
	mat_rotate_x(view, -cam_pitch);
	mat_rotate_z(view, -cam_yaw);

	// draw world

	glEnable(GL_DEPTH_TEST);

	draw_scene(scene, projection, view);

	glDisable(GL_DEPTH_TEST);

	// draw ui

	mat_ortho(projection, 0, screenw, screenh, 0, -1, 1);
	mat_identity(view);

	if (showconsole)
		console_draw(projection, view, droid_sans_mono, 15);

	glutSwapBuffers();

	gl_assert("swap buffers");
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 0);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitWindowPosition(50, 50+24);
	glutInitWindowSize(screenw, screenh);
	glutInitDisplayMode(GLUT_SRGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
	glutCreateWindow("Mio");

	gl3wInit();

	warn("OpenGL %s; GLSL %s", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);

	register_directory("data/");
	register_directory("data/textures/");

	droid_sans = load_font("fonts/DroidSans.ttf");
	if (!droid_sans)
		exit(1);

	droid_sans_mono = load_font("fonts/DroidSansMono.ttf");
	if (!droid_sans_mono)
		exit(1);

	console_init();

	bind_init();

	// load world
	scene = new_scene();
//	new_object(scene, "pr_s2_mycotree_a.iqe");
//	new_object(scene, "tr_s2_bamboo_a.iqe");
//	mat_translate(scene->objects->transform, 5, 0, 0);

	glutMainLoop();
	return 0;
}