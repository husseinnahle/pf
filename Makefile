CFLAGS = -Wall -Werror=vla -pedantic -std=c11
TIME_H = -D_POSIX_C_SOURCE=199309L
VAL = --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes

compile:
	gcc $(CFLAGS) $(TIME_H) pf.c -o pf

.PHONY: clean check check_local

clean:
	rm -rf pf

check: pf work
	bats check.bats
	
check_local: pf work
	bats local.bats
