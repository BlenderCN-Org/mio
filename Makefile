# GNU Makefile

build ?= debug

default: all

include Makerules

OUT := build/$(build)

ifeq "$(verbose)" ""
QUIET_CC = @ echo "   " CC $@ ;
QUIET_AR = @ echo "   " AR $@ ;
QUIET_LINK = @ echo "   " LINK $@ ;
endif

CC_CMD = $(QUIET_CC) $(CC) -o $@ -c $< $(CFLAGS)
AR_CMD = $(QUIET_AR) $(AR) cru $@ $^
LINK_CMD = $(QUIET_LINK) $(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

LUA_DIR := lua-5.2.1/src
LUA_SRC := \
	lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
	lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c \
	ltm.c lundump.c lvm.c lzio.c \
	lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c \
	lmathlib.c loslib.c lstrlib.c ltablib.c loadlib.c linit.c
LUA_OBJ := $(addprefix $(OUT)/, $(LUA_SRC:%.c=%.o))
LUA_LIB := $(OUT)/liblua.a

MIO_HDR := getopt.h iqm.h mio.h stb_truetype.h stb_image.c
MIO_SRC := \
	cache.c console.c draw.c font.c gl3w.c image.c \
	model.c model_iqe.c model_iqm.c model_obj.c \
	rune.c shader.c strlcpy.c vector.c zip.c
MIO_OBJ := $(addprefix $(OUT)/, $(MIO_SRC:%.c=%.o))
MIO_LIB := $(OUT)/libmio.a

$(OUT) :
	mkdir -p $@

$(OUT)/%.o : %.c $(MIO_HDR)
	$(CC_CMD) -I$(LUA_DIR)

$(OUT)/%.o : $(LUA_DIR)/%.c
	$(CC_CMD) -DLUA_COMPAT_ALL

$(LUA_LIB) : $(LUA_OBJ)
	$(AR_CMD)

$(MIO_LIB) : $(MIO_OBJ)
	$(AR_CMD)

a.exe : $(OUT)/a.o $(MIO_LIB) $(LUA_LIB)
	$(LINK_CMD)

all: $(OUT) $(LUA_LIB) $(MIO_LIB) a.exe

clean:
	rm -rf $(OUT)
