include config.mk

SRC = drw.c jwm.c bar.c util.c window.c event.c screen.c atom.c key.c client.c cursor.c font.c
OBJ = ${SRC:.c=.o}

all: jwm

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

jwm: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f jwm ${OBJ}

.PHONY: all clean
