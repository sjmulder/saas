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
	install -d $(bindir) $(man1dir)
	install saas $(bindir)/
	install saas.1 $(man1dir)/

uninstall:
	rm -f $(bindir)/saas
	rm -f $(man1dir)/saas.1

.PHONY: all clean
