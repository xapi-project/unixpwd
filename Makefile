#

CC 		= gcc
CFLAGS 	= -g -DMAIN
VOPTS   += --track-fds=yes
VOPTS   += --tool=memcheck
VOPTS   += --leak-check=full
VOPTS   += --show-leak-kinds=all
VOPTS   += --track-origins=yes

.PHONY: all clean indent

all:
		$(CC) $(CFLAGS) -o opasswd opasswd.c

clean:
		rm -f opasswd opasswd.o
		rm -f pwd.cma pwd.cmi pwd.cmo libpwd.a myocaml

myocaml:  opasswd.o pwd.cmo libpwd.a
		ocamlmktop -I . -o myocaml -custom pwd.cmo libpwd.a

opasswd.o: opasswd.c
		ocamlc -c $<

libpwd.a: 	opasswd.o
		ar cr $@ opasswd.o

pwd.cmo: 	pwd.ml
		ocamlc -c -o $@ $<

indent:
		indent -orig -nut opasswd.c

valgrind: opasswd
		valgrind $(VOPTS) ./opasswd root
		valgrind $(VOPTS) ./opasswd root foo
		valgrind $(VOPTS) ./opasswd > /dev/null
