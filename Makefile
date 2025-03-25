all: shell

clean:
	rm -vf shell utils.o

CPPFLAGS := -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS := -Wall -Wextra -std=c11 -pedantic -Werror

shell: shell.o utils.o
