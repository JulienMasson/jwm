include config.mk

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,obj/%.o,$(SRCS))

all: jwm

jwm: ${OBJS}
	@echo CC -o $@
	@${CC} -o $@ $^ ${LDFLAGS}

obj/%.o : %.c
	@mkdir -p obj
	@echo CC $^
	@${CC} -c ${CFLAGS} -o obj/$*.o $<

clean:
	@echo cleaning
	@rm -f jwm
	@rm -rf obj

.PHONY: all clean
