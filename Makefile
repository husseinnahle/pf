CFLAGS = -Wall -Werror=vla -pedantic -std=c11
VAL = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes

# make: compiler pf.c et creer penv
compile:
	gcc $(CFLAGS) pf.c -o pf

.PHONY: clean check check_local

clean:
	rm -rf pf

check: pf work
	bats check.bats
	
check_local: pf work
	bats local.bats
