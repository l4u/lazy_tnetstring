//
// Testing tnetstring library
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LTNSTerm.h"

#define min(a,b) ((a) < (b) ? a : b)

// setup function for tests
void setup_test();
void cleanup_test();

// define tests
int test_new();
int test_new_invalid();

int test_value_int();
int test_value_bool_true();
int test_value_bool_false();
int test_value_null(); 
int test_value_list();
int test_value_dictionary();

int test_value_undefined();

// c-tests only
int test_value_null_bytes();
int test_value_get_with_null_arguments();

// use tests
typedef int (*test_function)();

typedef struct {
	test_function function;
	const char description[128];
} test_case;

test_case tests[] = 
{
	/*  0 */ {test_new, "#new"},
	/*  1 */ {test_new_invalid, "non-tnestring-data"},

	/*  2 */ {test_value_int,"integer value"},
        /*  3 */ {test_value_bool_true, "boolean value of true" },
        /*  4 */ {test_value_bool_false, "boolean value of false" },
	/*  5 */ {test_value_null, "null value" },
        /*  6 */ {test_value_list, "list value" },
        /*  7 */ {test_value_dictionary, "dictionary" },

        /*  8 */ {test_value_undefined, "type error" },

	/*  9 */ {test_value_null_bytes, "value containing null bytes" },
	/* 10 */ {test_value_get_with_null_arguments, "value containing null bytes" },
};

// main function starting the test suite
int main( int argc, char **argv)
{
	int starting_test = 0;
	
	// create array for saving failed tests
	int test_count = sizeof(tests)/sizeof(test_case);

	switch( argc )
	{
		case 1:
			break;

		case 3: 
			test_count = min( test_count, atoi(argv[2])+1);
		case 2:
			starting_test = atoi(argv[1]);
			break;

		default:
			fprintf(stderr, "usage %s [FIRST_TEST] [LAST_TEST]\n", argv[0] );
			return -1;
			break;
	}

	int *failed_tests = (int*)malloc(sizeof(int) * test_count);
	int last_failed_test = 0;

	if( starting_test < test_count ) 
	{
		printf("Testing: ");
	}
	else
	{
		printf("No tests selected, exiting.\n");
	}

	// loop through all tests
	for(int test_id = starting_test; test_id < test_count; ++test_id)
	{
		setup_test();
		test_case test = tests[test_id];
		
		int result = test.function();
		if( !result )
		{
			printf( "\e[31mF\e[m", test_id );
			failed_tests[last_failed_test++] = test_id;
		}
		else
		{
			printf( "\e[32m.\e[m", test_id );
		}
		cleanup_test();

		fflush(stdout);
	}

	// print all failed tests
	if( last_failed_test != 0)
	{
		// display all failed tests
		printf("\n\nFollowing tests failed:\n");
		for( int failed_test = 0; failed_test < last_failed_test; ++failed_test )
		{
			int id = failed_tests[failed_test];
			printf("\t%3d: %s\n", id, tests[id].description );
		}
	}
	printf("\n");

	// cleanup
	free(failed_tests);
	return last_failed_test;
}

// space for global variables
LTNSTerm *subject;

// setup all tests
void setup_test()
{
	subject = (LTNSTerm*)calloc(1, sizeof(LTNSTerm) );
	LTNSCreateTerm( subject, "3:foo,", 6, LTNS_STRING);
}

void cleanup_test()
{
	LTNSDestroyTerm( subject );
	free(subject);
}


// declare tests
int test_new()
{
	char *data = subject->raw_data;
	size_t length = subject->raw_length;

	// raw data check
	if( !data || length != 6 || strcmp( data, "3:foo," ) != 0 )
	{
		return 0;
	}

	// payload check
	LTNSType type = LTNS_UNDEFINED;
	int result = LTNSGetPayloadType( subject, &type );
	result && (result = LTNSGetPayloadLength( subject, &length));
	return result && (length == 3) && (type == LTNS_STRING);
}

int test_new_invalid()
{
	// negative test for invalid data (expected to fail)
	LTNSDestroyTerm( subject );
	int result = LTNSCreateTerm( subject, NULL, 4711, LTNS_STRING);
	!result && (result = LTNSCreateTerm( subject, "foo", 3, LTNS_UNDEFINED));
	return !result;
}

static int check_raw( LTNSTerm *term, const char* expected_raw, size_t expected_raw_length )
{
	return term &&
		(term->raw_length == expected_raw_length) &&
		(strncmp(term->raw_data, expected_raw, expected_raw_length));
}

static int check_payload( LTNSTerm *term, const char* expected_payload, size_t expected_length, LTNSType expected_type )
{ 
	char *payload = NULL;
	size_t length = 0;
	LTNSType type = LTNS_UNDEFINED;
	int result = LTNSGetPayload( term, payload, &length, &type );
	LTNSType seperate_type = LTNS_UNDEFINED;
	result &= LTNSGetPayloadType( term, &seperate_type );
	
	return result && 
		(length == expected_length) && 
		(type == expected_type) &&
		(seperate_type == expected_type) &&
		(strcmp(payload, expected_payload) == 0);
}

int test_value_int()
{
	int result = LTNSDestroyTerm( subject );
	result && (result = LTNSCreateTerm( subject, "4711", 4, LTNS_INTEGER));
	result && (result = check_raw( subject, "4:4711#", 7 ));
	result && (result = check_payload( subject, "4711", 4, LTNS_INTEGER ));
	return result;
}

int test_value_bool_true()
{
	int result = LTNSDestroyTerm( subject );
	result && (result = LTNSCreateTerm( subject, "true", 4, LTNS_BOOLEAN));
	result && (result = check_raw( subject, "4:true!", 7 ));
	result && (result = check_payload( subject, "true", 4, LTNS_BOOLEAN ));
	return result;
}

int test_value_bool_false()
{
	int result = LTNSDestroyTerm( subject );
	result && (result = LTNSCreateTerm( subject, "false", 5, LTNS_BOOLEAN));
	result && (result = check_raw( subject, "5:false!", 8 ));
	result && (result = check_payload( subject, "false", 5, LTNS_BOOLEAN ));
	return result;
}

int test_value_null()
{
	int result = LTNSDestroyTerm( subject );
	result && (result = LTNSCreateTerm( subject, "", 0, LTNS_NULL));
	result && (result = check_raw( subject, "0:~", 3 ));
	result && (result = check_payload( subject, "", 0, LTNS_NULL ));
	return result;
}

int test_value_list()
{
	int result = LTNSDestroyTerm( subject );
	result && (result = LTNSCreateTerm( subject, "5:Hello,5:World,", 16, LTNS_LIST));
	result && (result = check_raw( subject, "16:5:Hello,5:World,]", 20 ));
	result && (result = check_payload( subject, "5:Hello,5:World,", 16, LTNS_LIST ));
	return result;
}

int test_value_dictionary()
{
	int result = LTNSDestroyTerm( subject );
	result && (result = LTNSCreateTerm( subject, "3:key,5:value,", 14, LTNS_DICTIONARY));
	result && (result = check_raw( subject, "13:3:key,5:value,]", 18 ));
	result && (result = check_payload( subject, "3:key,5:value,", 14, LTNS_DICTIONARY));
	return result;
}

int test_value_undefined()
{
	// negative check : it is expected to fail!
	int result = LTNSDestroyTerm( subject );
	result && (result = LTNSCreateTerm( subject, "3:foo/", 6, LTNS_STRING)); // correct type, incorect string
	!result && (result = LTNSCreateTerm( subject, "3:foo,", 6, LTNS_UNDEFINED)); // correct string, but incorrect type
	return !result;
}

int test_value_null_bytes()
{
	char *data = "\061\061\000\061\061"; // store a string containing NULL-Bytes
	int result = LTNSDestroyTerm( subject );
	result && (result = LTNSCreateTerm( subject, data, 5, LTNS_STRING ));
	return result && check_payload( subject, "aa\000aa", 5, LTNS_STRING );
}

int test_value_get_with_null_arguments()
{
	// getter test with unset output parameters
	char* payload = NULL;
	int result = LTNSGetPayload( subject, payload, NULL, NULL );
	return result && strcmp(payload, "foo");
}

