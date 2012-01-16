all : term_test data_acces_test

data_acces_test:
	gcc -std=c99 -o data_acces_test -g -I../src data_access_test.c ../src/*.c
term_test:
	gcc -std=c99 -o term_test -g -I../src term_test.c ../src/*.c

clean:
	rm *.o term_test data_access_test

