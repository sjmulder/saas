CFLAGS += -ansi
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -g

# BSD conventions by default, override to taste
prefix  ?= /usr/local
bindir  ?= $(prefix)/bin
man1dir ?= $(prefix)/man/man1

all: saas

clean:
	rm -f saas

install:
	install -d $(bindir)
	install saas $(bindir)/

uninstall:
	rm -f $(bindir)/saas

.PHONY: all clean
