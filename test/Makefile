CFLAGS = -I../ext/include -std=c99 -Wall -Werror -lm

all: term_test data_access_test

data_access_test: data_access_test.c
	gcc -o data_access_test test.c -DTEST_SUITE=\"data_access_test.c\" ../ext/LTNS*.c ${CFLAGS}
term_test: term_test.c
	gcc -o term_test test.c -DTEST_SUITE=\"term_test.c\" ../ext/LTNS*.c ${CFLAGS}

clean:
	rm -rf data_access_test term_test data_access_test.dSYM term_test.dSYM

