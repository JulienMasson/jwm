PREFIX?=/usr/local
LIB_SUFFIX?=lib
X11_INCLUDE?=/usr/local/include

CFLAGS+=-Os -std=c99 -s -I${X11_INCLUDE} -Wunused
LDFLAGS+=-L${PREFIX}/${LIB_SUFFIX} -lxcb -lxcb-randr -lxcb-keysyms \
		 -lxcb-icccm -lxcb-ewmh
TARGETS=jwm
OBJS=jwm.o

all: $(TARGETS)

jwm: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

jwm.o: jwm.c list.h config.h Makefile

clean:
	rm -f $(TARGETS) *.o
