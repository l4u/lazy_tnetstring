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
	result &= LTNSTermGetPayloadType( term, &seperate_type );
	
	return result && 
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
	LTNSTermCreate( &subject, "3:foo,", 6, LTNS_STRING);
}

void cleanup_test()
{
	LTNSTermDestroy( subject );
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
	int result = LTNSTermGetPayloadType( subject, &type );
	result && (result = LTNSTermGetPayloadLength( subject, &length));
	return result && (length == 3) && (type == LTNS_STRING);
}

int test_new_invalid()
{
	// negative test for invalid data (expected to fail)
	LTNSTermDestroy( subject );
	int result = LTNSTermCreate( &subject, NULL, 4711, LTNS_STRING);
	!result && (result = LTNSTermCreate( &subject, "foo", 3, LTNS_UNDEFINED));
	return !result;
}

int test_value_int()
{
	int result = LTNSTermDestroy( subject );
	result && (result = LTNSTermCreate( &subject, "4711", 4, LTNS_INTEGER));
	result && (result = check_raw( subject, "4:4711#", 7 ));
	result && (result = check_payload( subject, "4711", 4, LTNS_INTEGER ));
	return result;
}

int test_value_bool_true()
{
	int result = LTNSTermDestroy( subject );
	result && (result = LTNSTermCreate( &subject, "true", 4, LTNS_BOOLEAN));
	result && (result = check_raw( subject, "4:true!", 7 ));
	result && (result = check_payload( subject, "true", 4, LTNS_BOOLEAN ));
	return result;
}

int test_value_bool_false()
{
	int result = LTNSTermDestroy( subject );
	result && (result = LTNSTermCreate( &subject, "false", 5, LTNS_BOOLEAN));
	result && (result = check_raw( subject, "5:false!", 8 ));
	result && (result = check_payload( subject, "false", 5, LTNS_BOOLEAN ));
	return result;
}

int test_value_null()
{
	int result = LTNSTermDestroy( subject );
	result && (result = LTNSTermCreate( &subject, "", 0, LTNS_NULL));
	result && (result = check_raw( subject, "0:~", 3 ));
	result && (result = check_payload( subject, "", 0, LTNS_NULL ));
	return result;
}

int test_value_list()
{
	int result = LTNSTermDestroy( subject );
	result && (result = LTNSTermCreate( &subject, "5:Hello,5:World,", 16, LTNS_LIST));
	result && (result = check_raw( subject, "16:5:Hello,5:World,]", 20 ));
	result && (result = check_payload( subject, "5:Hello,5:World,", 16, LTNS_LIST ));
	return result;
}

int test_value_dictionary()
{
	int result = LTNSTermDestroy( subject );
	result && (result = LTNSTermCreate( &subject, "3:key,5:value,", 14, LTNS_DICTIONARY));
	result && (result = check_raw( subject, "13:3:key,5:value,]", 18 ));
	result && (result = check_payload( subject, "3:key,5:value,", 14, LTNS_DICTIONARY));
	return result;
}

int test_value_undefined()
{
	// negative check : it is expected to fail!
	int result = LTNSTermDestroy( subject );
	result && (result = LTNSTermCreate( &subject, "foo", 23, LTNS_STRING)); // correct type, incorect length
	!result && (result = LTNSTermCreate( &subject, "foo", 3, LTNS_UNDEFINED)); // correct string, but incorrect type
	return !result;
}

int test_value_copy()
{
	char *payload = "foo";

	LTNSTerm *copy = NULL;
	LTNSTermCreate( &copy, payload, 3, LTNS_STRING);

	char* returned_payload = NULL;
	LTNSTermGetPayload( copy, &returned_payload, NULL, NULL );

	return payload != returned_payload;
}

int test_value_nested()
{
	char *data = "3:foo,";

	LTNSTerm *nested = NULL;
	LTNSTermCreateNested( &nested, data, 6, LTNS_STRING);

	return data == nested->raw_data;
}

int test_value_null_bytes()
{
	char *data = "\061\061\000\061\061"; // store a string containing NULL-Bytes
	int result = LTNSTermDestroy( subject );
	result && (result = LTNSTermCreate( &subject, data, 5, LTNS_STRING ));
	return result && check_payload( subject, "aa\000aa", 5, LTNS_STRING );
}

int test_value_get_with_null_arguments()
{
	// getter test with unset output parameters
	char* payload = NULL;
	int result = LTNSTermGetPayload( subject, &payload, NULL, NULL );
	return result && strcmp(payload, "foo");
}

