//
// Base include file for all test suites
//


#ifndef _TEST_SUITE_H__
#define _TEST_SUITE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LTNSTerm.h"

#define min(a,b) ((a) < (b) ? a : b)

// setup function for tests
void setup_test();
void cleanup_test();

// use tests
typedef int (*test_function)();

typedef struct {
	test_function function;
	const char description[128];
} test_case;

// define this global and add your tests
extern test_case tests[];

#endif //  _TEST_SUITE_H__
