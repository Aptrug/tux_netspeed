.PHONY: all install uninstall clean

PREFIX ?= ~/.local
BINDIR := ${PREFIX}/bin
CFLAGS := -Wall -Wextra -pedantic -ansi

all:
	${CC} ${CFLAGS} tux_netspeed.c -o tux_netspeed

install: all
	mkdir -p ${DESTDIR}${BINDIR}
	install -m 755 tux_netspeed ${DESTDIR}${BINDIR}

uninstall:
	rm -f ${DESTDIR}${BINDIR}/tux_netspeed

clean:
	rm -f tux_netspeed
