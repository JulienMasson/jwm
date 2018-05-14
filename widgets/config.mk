# paths
WIDGETS_DIR := widgets

# widgets cflags
CFLAGS_WIDGETS := $(CFLAGS)
CFLAGS_WIDGETS += -fPIC --shared

# source and shared libraries
WIDGETS_SRC := $(wildcard $(WIDGETS_DIR)/*.c)
WIDGETS_SHARED_OBJ := $(WIDGETS_SRC:.c=.so)

# widgets target
$(WIDGETS_DIR)/%.so: $(WIDGETS_DIR)/%.c
	${CC} $(CFLAGS_WIDGETS) $< -o $@

widgets: $(WIDGETS_SHARED_OBJ)
