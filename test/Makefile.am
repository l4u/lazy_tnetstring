AM_CFLAGS = -I../src -L../src -lLTNS -std=c99

bin_PROGRAMS = term_test data_access_test

term_test_CFLAGS = -DTEST_SUITE=\"term_test.c\" -I../src -L../src -lLTNS -std=c99
term_test_SOURCES = test.c

data_access_test_SOURCES = test.c
data_access_test_CFLAGS = -DTEST_SUITE=\"data_access_test.c\" -I../src -L../src -lLTNS -std=c99

