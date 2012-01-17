//
// Testing tnetstring library
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LTNSTerm.h"
#include "test_suite.h"

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
int test_value_nested();
int test_value_copy();
int test_value_null_bytes();
int test_value_get_with_null_arguments();

// utility tests
int test_count_digits();

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

	/*  9 */ {test_value_nested, "constructor with raw data access, for nested data, and no memory allocation" },
	/* 10 */ {test_value_copy, "copies the given payload into newly created data" },
	/* 11 */ {test_value_null_bytes, "value containing null bytes" },
	/* 12 */ {test_value_get_with_null_arguments, "value containing null bytes" },

	/* 13 */ { test_count_digits, "value test" },
};

// helper functions
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
	int result = LTNSTermGetPayload( term, &payload, &length, &type );
	LTNSType seperate_type = LTNS_UNDEFINED;
	result |= LTNSTermGetPayloadType( term, &seperate_type );
	
	return result == 0 && 
		(length == expected_length) && 
		(type == expected_type) &&
		(seperate_type == expected_type) &&
		(strcmp(payload, expected_payload) == 0);
}

// space for global variables
LTNSTerm *subject;

// setup all tests
void setup_test()
{
	subject = (LTNSTerm*)calloc(1, sizeof(LTNSTerm) );
	assert( subject != NULL );
	assert( 0 == LTNSTermCreate( &subject, "foo", 3, LTNS_STRING) );
}

void cleanup_test()
{
	assert( 0 == LTNSTermDestroy( subject ) );
	free(subject);
}

// declare tests
int test_new()
{
	char *data = subject->raw_data;
	size_t length = subject->raw_length;

	// raw data check
	assert( !data || length != 6 || strcmp( data, "foo" ) != 0 );

	// payload check
	LTNSType type = LTNS_UNDEFINED;
	int result = LTNSTermGetPayloadType( subject, &type );
	assert( 0 == result );
	assert( type == LTNS_STRING );

	assert( 0 == LTNSTermGetPayloadLength( subject, &length) );
	assert( 3 == length );
	return result == 0;
}

int test_new_invalid()
{
	// negative test for invalid data (expected to fail)
	LTNSTermDestroy( subject );
	int result = LTNSTermCreate( &subject, NULL, 4711, LTNS_STRING);
	assert( 0 != (result = LTNSTermCreate( &subject, "foo", 3, LTNS_UNDEFINED)));
	return result != 0;
}

int test_value_int()
{
	int result = LTNSTermDestroy( subject );
	result = LTNSTermCreate( &subject, "4711", 4, LTNS_INTEGER);
	result = check_raw( subject, "4:4711#", 7 );
	result = check_payload( subject, "4711", 4, LTNS_INTEGER );
	return result == 0;
}

int test_value_bool_true()
{
	int result = LTNSTermDestroy( subject );
	result = LTNSTermCreate( &subject, "true", 4, LTNS_BOOLEAN);
	result = check_raw( subject, "4:true!", 7 );
	result = check_payload( subject, "true", 4, LTNS_BOOLEAN );
	return result == 0;
}

int test_value_bool_false()
{
	int result = LTNSTermDestroy( subject );
	result = LTNSTermCreate( &subject, "false", 5, LTNS_BOOLEAN);
	result = check_raw( subject, "5:false!", 8 );
	result = check_payload( subject, "false", 5, LTNS_BOOLEAN );
	return result == 0;
}

int test_value_null()
{
	int result = LTNSTermDestroy( subject );
	assert( 0 == (result = LTNSTermCreate( &subject, NULL, 0, LTNS_NULL) ));
	assert( 0 == (result = check_raw( subject, "0:~", 3 )));
	assert( 0 == (result = check_payload( subject, NULL, 0, LTNS_NULL )));
	return result == 0;
}

int test_value_list()
{
	int result = LTNSTermDestroy( subject );
	assert( 0 == (result = LTNSTermCreate( &subject, "5:Hello,5:World,", 16, LTNS_LIST)));
	assert( 0 == (result = check_raw( subject, "16:5:Hello,5:World,]", 20 )));
	assert( 0 == (result = check_payload( subject, "5:Hello,5:World,", 16, LTNS_LIST )));
	return result == 0;
}

int test_value_dictionary()
{
	int result = LTNSTermDestroy( subject );
	assert( 0 == (result = LTNSTermCreate( &subject, "3:key,5:value,", 14, LTNS_DICTIONARY)));
	assert( 0 == (result = check_raw( subject, "13:3:key,5:value,]", 18 )));
	assert( 0 == (result = check_payload( subject, "3:key,5:value,", 14, LTNS_DICTIONARY)));
	return result == 0;
}

int test_value_undefined()
{
	// negative check : it is expected to fail!
	int result = LTNSTermDestroy( subject );
	assert( 0 != (result = LTNSTermCreate( &subject, "foo", 3, LTNS_UNDEFINED))); // correct string, but incorrect type
	return result != 0;
}

int test_value_copy()
{
	char *payload = "foo";

	LTNSTerm *copy = NULL;
	assert( 0 == (LTNSTermCreate( &copy, payload, 3, LTNS_STRING) ));

	char* returned_payload = NULL;
	assert( 0 == (LTNSTermGetPayload( copy, &returned_payload, NULL, NULL )) );

	return payload != returned_payload;
}

int test_value_nested()
{
	char *data = "3:foo,";

	LTNSTerm *nested = NULL;
	assert( 0 == LTNSTermCreateNested( &nested, data, 6) );

	return data == nested->raw_data;
}

int test_value_null_bytes()
{
	char *data = "\061\061\000\061\061"; // store a string containing NULL-Bytes
	int result = LTNSTermDestroy( subject );
	result = LTNSTermCreate( &subject, data, 5, LTNS_STRING );
	return result == 0 && check_payload( subject, "aa\000aa", 5, LTNS_STRING );
}

int test_value_get_with_null_arguments()
{
	// getter test with unset output parameters
	char* payload = NULL;
	int result = LTNSTermGetPayload( subject, &payload, NULL, NULL );
	return result == 0 && strcmp(payload, "foo") == 0;
}

int test_count_digits()
{
	assert( count_digits(  1) == 1 );
	assert( count_digits( 10) == 2 );
	assert( count_digits(100) == 3 );
	assert( count_digits( 0 ) == 1 );
	assert( count_digits(-1 ) == 0 ); // error case
	assert( count_digits(-0 ) == 1 );
	return 1;
}
