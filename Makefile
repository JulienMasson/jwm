# paths
OBJ_DIR := obj
SRC_DIR := src
ROOT_DIR := $(shell pwd)

# wallpaper
WALLPAPER_PATH := $(ROOT_DIR)/res/wallpaper.png
WALLPAPER_FLAGS := -D DEFAULT_WALLPAPER='"${WALLPAPER_PATH}"'

# icons DIR
ICONS_DIR := $(ROOT_DIR)/res/icons/
ICONS_FLAGS := -D ICONS_DIR='"${ICONS_DIR}"'

# flags
CFLAGS := -Werror -Wall -Wextra -O0 -g ${WALLPAPER_FLAGS} ${ICONS_FLAGS} -export-dynamic
LDFLAGS := -pthread -ldl

# libs and include
pkg_configs := xcb \
               xcb-randr \
               xcb-keysyms \
               xcb-icccm \
               xcb-ewmh \
               xcb-aux \
               cairo \
               pangocairo

LIBS := $(shell pkg-config --libs   ${pkg_configs})
INCS := $(shell pkg-config --cflags ${pkg_configs})

CFLAGS += -I${SRC_DIR} ${INCS}
LDFLAGS += ${LIBS}

# source and objects
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# targets
TARGET := jwm

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	${CC} -o $@ -c $< ${CFLAGS}

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET) $(OBJ)

# test suite
include test/config.mk
