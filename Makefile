DESTDIR   ?=
PREFIX    ?= /usr/local
MANPREFIX ?= $(PREFIX)/man

CFLAGS += -ansi -D_DEFAULT_SOURCE
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -g

# BSD conventions by default, override to taste
prefix  ?= /usr/local
bindir  ?= $(prefix)/bin
man1dir ?= $(prefix)/man/man1

all: saas

clean:
	rm -f saas

install: saas
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(MANPREFIX)/man1
	install saas   $(DESTDIR)$(PREFIX)/bin/
	install saas.1 $(DESTDIR)$(MANPREFIX)/man1/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/saas
	rm -f $(DESTDIR)$(MANPREFIX)/man1/saas.1

.PHONY: all clean install uninstall
