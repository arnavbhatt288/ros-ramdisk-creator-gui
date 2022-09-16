# Install
CC=gcc

BIN = ramdisk-creator

OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
	CFLAGS += -std=c99 -pedantic -O2 -lcomctl32 -Wl,--subsystem,windows -lgdi32 -lole32 -lcomdlg32 -lmsimg32
	SRC = main.c gdi.c gui.c utils_win.c tinyfiledialogs.c common.c
else
	CFLAGS += -std=c99 -pedantic -O2 -lX11 -lm -lblkid -lpthread
	SRC = main.c x11.c gui.c utils_linux.c tinyfiledialogs.c common.c
endif

$(BIN):
	$(CC) $(SRC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -o $(BIN)
