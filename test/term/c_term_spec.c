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
int test_value_bool();
int test_value_null(); 
int test_value_list();
int test_value_dictionary();

int test_value_undefined();

// use tests
typedef int (*test_function)();

typedef struct {
	test_function function;
	const char description[128];
} test_case;

test_case tests[] = 
{
	{test_new, "#new"},
	{test_new_invalid, "non-tnestring-data"},

	{test_value_int,"integer value"},
        {test_value_bool, "boolean value" },
	{test_value_null, "null value" },
        {test_value_list, "list value" },
        {test_value_dictionary, "dictionary" },

        {test_value_undefined, "type error" },
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
		if( !test.function() )
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
			printf("\t%d: %s\n", id, tests[id].description );
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
	LTNSCreateTerm( &subject );
	LTNSSetData( subject, "3:foo,", 6);
}

void cleanup_test()
{
	LTNSDestroyTerm( subject );
}



// declare tests
int test_new()
{
	char *data;
	size_t length = 0;
	int result = LTNSGetData( subject, data, &length );

	if( !result || length != 6 || strcmp( data, "3:foo," ) != 0 )
	{
		return 0;
	}

	result = LTNSGetValueLength( subject, &length);
	return result && length == 3;
}

int test_new_invalid()
{
	// negative test for invalid data
	int result = LTNSSetData( subject, "12345",5 );

	char data[10];
	size_t length = 0;
	LTNSGetString(subject, data, &length);
	return !result && strcmp(data, "12345") != 0 && length != 5;
}

int test_value_int()
{
	int data = 4711;
	LTNSSetInteger( subject, data );
	data = 0;

	int result = LTNSGetInteger( subject, &data );
	return result && LTNSGetType( subject ) == LTNS_INTEGER && data == 4711;
}

int test_value_bool()
{
	int data = 1;
	LTNSSetBoolean( subject, data );
	data = 0;

	int result = LTNSGetBoolean( subject, &data );
	return result && LTNSGetType( subject ) == LTNS_BOOLEAN && data == 1;
}

int test_value_null()
{
	int result = LTNSSetNull( subject );
	char* raw_string;
	size_t length;
	LTNSGetData( subject, raw_string, &length );
	return result && LTNSGetType(subject ) == LTNS_NULL && strcmp( raw_string, "0:~");
}

int test_value_list()
{
	char *entries = "0:~2:23#3:foo,";
	int result = LTNSSetList( subject, entries, strlen(entries) );
	return result && LTNSGetType( subject ) == LTNS_LIST;
}

int test_value_dictionary()
{
	char *entries = "3:one,1:1#3:two,1:2#"; // maps one => 1 and two => 2
	int result = LTNSSetDictionary( subject, entries, strlen(entries) );
	return result && LTNSGetType(subject) == LTNS_DICTIONARY;
}

int test_value_undefined()
{
	// should not work '/' is no type!
	int result = LTNSSetData(subject, "3:foo/", 6);
	return result == 0 && LTNSGetType(subject) == LTNS_UNDEFINED;
}


