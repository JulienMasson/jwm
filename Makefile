# paths
OBJ_DIR := obj
SRC_DIR := src

# flags
DEBUG := 0
ifeq ($(DEBUG), 1)
CFLAGS := -Wunused -O0 -g
else
CFLAGS := -Wunused -Os
endif
LDFLAGS :=

# libs and include
pkg_configs := xcb \
               xcb-randr \
               xcb-keysyms \
               xcb-icccm \
               xcb-ewmh \
               xcb-aux \
               cairo

LIBS := $(shell pkg-config --libs   ${pkg_configs})
INCS := $(shell pkg-config --cflags ${pkg_configs})

CFLAGS += -I. ${INCS}
LDFLAGS += -L. ${LIBS}

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
