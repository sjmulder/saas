CFLAGS += -ansi
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -g

all: saas

clean:
	rm -f saas

.PHONY: all clean
