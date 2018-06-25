#

CC 		= gcc
CFLAGS 	= -g -DDEVELOPMENT

VOPTS   += --track-fds=yes
VOPTS   += --tool=memcheck
VOPTS   += --leak-check=full
VOPTS   += --show-leak-kinds=all
VOPTS   += --track-origins=yes

.PHONY: all clean indent valgrind

all: 	unixpwd myocaml

unixpwd: main.o unixpwd.o
		$(CC) $(CFLAGS) -o unixpwd unixpwd.o main.o

clean:
		rm -f unixpwd
		rm -f *.o
		rm -f unixpwd.cma unixpwd.cmi unixpwd.cmo libunixpwd.a myocaml
		rm -f shadow.* passwd.*

myocaml:  unixpwd.cmo libunixpwd.a
		ocamlmktop -I . -o myocaml -custom $^

unixpwd_stub.o: unixpwd_stub.c unixpwd.h
		ocamlc -c $<

libunixpwd.a: 	unixpwd.o unixpwd_stub.o
		ar cr $@ $^

unixpwd.cmo: unixpwd.ml
		ocamlc -c -o $@ $<

indent:
		indent -orig -nut unixpwd.c unixpwd.h unixpwd_stub.c

valgrind: unixpwd
		valgrind $(VOPTS) ./unixpwd root
		valgrind $(VOPTS) ./unixpwd root foo
		valgrind $(VOPTS) ./unixpwd > /dev/null
