#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "LTNSTerm.h"
#include "LTNSDataAccess.h"

#include "test_suite.h"

// define tests
int test_create_non_compliant();
int test_create_non_hash_toplevel();
int test_create_empty();
int test_create();
int test_create_with_parent();
int test_create_with_scope();
/* get */
int test_get_empty();
int test_get_unknown();
int test_get_known();
int test_get_nested();
int test_get_unknown_nested();
/* set */
int test_set_same_length();
int test_set_longer();
int test_set_shorter();
int test_set_nested_same_length();
int test_set_nested_longer();
int test_set_nested_shorter();
int test_set_multiple();
int test_set_multiple_scope();
int test_set_multiple_scope_interleaved();
int test_set_invalidating_scope();
/* add */
int test_set_empty();
int test_set_non_empty();
int test_set_nested();
/* remove */
int test_set_known_null();
int test_set_unknown_null();
int test_set_nested_null();

test_case tests[] = 
{
	/* create */
	{test_create_non_compliant, "create with non-tnetstring compliant data"},
	{test_create_non_hash_toplevel, "create with anything but a hash at the top level"},
	{test_create_empty, "create with an empty hash"},
	{test_create, "create with a hash"},
	{test_create_with_parent, "create with parent"},
	/* get */
	{test_get_empty, "get from empty hash"},
	{test_get_unknown, "get with unknown key"},
	{test_get_known, "get with known key"},
	{test_get_nested, "get key from nested hash"},
	{test_get_unknown_nested, "get unknown key from nested hash"},
	/* set */
	{test_set_same_length, "set without changing length"},
	{test_set_longer, "set with changing the length to longer"},
	{test_set_shorter, "set with changing the length to shorter"},
	{test_set_nested_same_length, "set with same length for nested key"},
	{test_set_nested_longer, "set with new longer length for nested key"},
	{test_set_nested_shorter, "set with new shorter length for nested key"},
	{test_set_multiple, "set with multiple new values on different levels"},
	{test_set_multiple_scope, "set with multiple new values on different levels while re-using scoped access"},
	{test_set_multiple_scope_interleaved, "set with multiple interleaved new values on different levels while re-using scoped access"},
	{test_set_invalidating_scope, "set inner key so that old references get invalidated"},
	/* add */
	{test_set_empty, "set a new key in an empty hash"},
	{test_set_non_empty, "set a new key in an non empty hash"},
	{test_set_nested, "set a new key in an nested hash"},
	/* remove */
	{test_set_known_null, "set a known key's value to null"},
	{test_set_unknown_null, "set a unknown key's value to null"},
	{test_set_nested_null, "set a known nested key's value to null"}
};

void setup_test()
{
}

void cleanup_test()
{
}

static LTNSDataAccess* new_data_access(const char* tnetstring)
{
	LTNSDataAccess* data_access = NULL;
	LTNSError error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	return data_access;
}

static LTNSDataAccess* new_nested_data_access(LTNSDataAccess* parent, LTNSTerm* term)
{
	LTNSDataAccess* data_access = NULL;
	LTNSError error;

	error = LTNSDataAccessCreateNested(&data_access, parent, term);
	assert(!error);
	assert(data_access != NULL);

	return data_access;
}

static LTNSTerm* get_term(LTNSDataAccess* data_access, const char* key)
{
	LTNSTerm* term = NULL;
	LTNSError error = LTNSDataAccessGet(data_access, key, &term);
	assert(!error);
	assert(term != NULL);
	return term;
}

static int check_term(LTNSTerm* term, const char* expected_payload, size_t expected_payload_length, LTNSType expected_type)
{
	char* payload;
	size_t payload_length;
	LTNSType type;

	LTNSError error = LTNSTermGetPayload(term, &payload, &payload_length, &type);
	assert(!error);
	return error == 0 &&
		type == expected_type &&
		payload_length == expected_payload_length &&
		memcmp(payload, expected_payload, payload_length) == 0;
}

static LTNSTerm* new_term(char* payload)
{
	LTNSTerm* term = NULL;
	LTNSError error = LTNSTermCreate(&term, payload, strlen(payload), LTNS_STRING);
	assert(!error);
	assert(term != NULL);
	assert(check_term(term, payload, strlen(payload), LTNS_STRING));

	return term;
}

static int set_key(LTNSDataAccess* data_access, const char* key, LTNSTerm* term)
{
	LTNSError error;

	assert(data_access);
	assert(key);
	assert(term);

	error = LTNSDataAccessSet(data_access, key, term);
	assert(!error);
	
	return TRUE;
}

static int set_and_check(LTNSDataAccess* data_access, const char* key, char* payload, size_t length, LTNSType type)
{
	LTNSTerm *term = NULL, *after_set = NULL;

	term = new_term(payload);
	assert(set_key(data_access, key, term));
	assert(!LTNSTermDestroy(term));

	after_set = get_term(data_access, key);
	assert(check_term(after_set, payload, length, type));
	assert(!LTNSTermDestroy(after_set));

	return TRUE;
}

int test_create_non_compliant()
{
	LTNSDataAccess *data_access = NULL;
	LTNSError error;
	const char* tnetstring = "12345";

	/* for non-tnetstring compliant data */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(error == INVALID_TNETSTRING);
	assert(data_access == NULL);

	return 1;
}

int test_create_non_hash_toplevel()
{
	LTNSDataAccess *data_access = NULL;
	LTNSError error;
	const char* tnetstring = "1:1#";

	/* for anything but a hash at the top level */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(error == UNSUPPORTED_TOP_LEVEL_DATA_STRUCTURE);
	assert(data_access == NULL);

	return 1;
}

int test_create_empty()
{
	LTNSDataAccess *data_access = NULL;
	const char* tnetstring = "0:}";

	/* for an empty hash */
	data_access = new_data_access(tnetstring);
	assert(data_access);
	size_t offset = 0;
	assert(!LTNSDataAccessOffset(data_access, &offset));
	assert(offset == 0);
	
	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_create()
{
	LTNSDataAccess *data_access = NULL;
	const char* tnetstring = "32:3:key,5:value,7:another,5:value,}";

	/* for a hash */
	data_access = new_data_access(tnetstring);
	assert(data_access);
	size_t offset = 0;
	assert(!LTNSDataAccessOffset(data_access, &offset));
	assert(offset == 0);

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_create_with_parent()
{
	LTNSDataAccess *data_access = NULL, *parent;
	LTNSError error;
	const char* tnetstring = "9:3:key,0:}}";

	/* with parent */
	parent = new_data_access(tnetstring);
	LTNSTerm *child_term;
	LTNSDataAccessGet( parent, "key", &child_term );
	error = LTNSDataAccessCreateNested(&data_access, parent, child_term);
	assert(!error);
	assert(!LTNSTermDestroy(child_term));
	assert(data_access);
	size_t offset = 0;
	assert(!LTNSDataAccessOffset(data_access, &offset));
	assert(offset == 8);
	
	LTNSDataAccess *returned_parent = NULL;
	assert(!LTNSDataAccessParent(data_access, &returned_parent));
	assert(returned_parent == parent);
	
	LTNSChildNode *first = NULL;
	assert(!LTNSDataAccessChildren(parent, &first));
	assert(first->child == data_access);
	assert(first->next == NULL);

	assert(!LTNSDataAccessDestroy(parent));
	return 1;
}

int test_get_empty()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	LTNSError error;
	const char* tnetstring = "0:}";

	/* for an empty hash */
	data_access = new_data_access(tnetstring);
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(error == KEY_NOT_FOUND);
	assert(term == NULL);

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_get_unknown()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	LTNSError error;
	const char* tnetstring = "12:3:baz,3:bar,}";

	/* for unknown keys */
	data_access = new_data_access(tnetstring);
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(error == KEY_NOT_FOUND);
	assert(term == NULL);

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_get_known()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	const char* tnetstring = "12:3:foo,3:bar,}";

	/* for known keys */
	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "foo");
	assert(check_term(term, "bar", 3, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_get_nested()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	const char* tnetstring = "28:5:outer,16:5:inner,5:value,}}";

	/* for nested hash */
	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "outer");
	assert(check_term(term, "5:inner,5:value,", 16, LTNS_DICTIONARY));

	inner = new_nested_data_access(data_access, term);
	assert(!LTNSTermDestroy(term));
	term = get_term(inner, "inner");
	assert(check_term(term, "value", 5, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_get_unknown_nested()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	LTNSError error;
	const char* tnetstring = "38:5:outer,16:5:inner,5:value,}4:user,0:}}";

	/* for nested hash with non-existing key */
	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "outer");
	inner = new_nested_data_access(data_access, term);
	assert(!LTNSTermDestroy(term));

	error = LTNSDataAccessGet(inner, "non-existing-key", &term);
	assert(error == KEY_NOT_FOUND);
	assert(term == NULL);

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_set_same_length()
{
	LTNSDataAccess *data_access = NULL;
	
	const char* tnetstring = "12:3:foo,3:bar,}";

	/* whithout changing length */
	data_access = new_data_access(tnetstring);
	assert(set_and_check(data_access, "foo", "baz", 3, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_set_longer()
{
	LTNSDataAccess *data_access = NULL;
	const char* tnetstring = "12:3:foo,3:bar,}";

	/* when changing the length of top-level key */
	data_access = new_data_access(tnetstring);
	assert(set_and_check(data_access, "foo", "foobar", 6, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_set_shorter()
{
	LTNSDataAccess *data_access = NULL;
	const char* tnetstring = "15:3:foo,6:foobar,}";

	/* when changing the length of top-level key */
	data_access = new_data_access(tnetstring);
	assert(set_and_check(data_access, "foo", "bar", 3, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}


int test_set_nested_same_length()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	const char* tnetstring = "24:5:outer,12:3:foo,3:bar,}}";

	/* when changing a nested key's value without changing the length */
	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "outer");
	inner = new_nested_data_access(data_access, term);
	assert(set_and_check(inner, "foo", "baz", 3, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_set_nested_longer()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	const char* tnetstring = "24:5:outer,12:3:foo,3:bar,}}";

	/* when changing the length of a nested key's value */
	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "outer");
	inner = new_nested_data_access(data_access, term);
	assert(set_and_check(inner, "foo", "foobar", 6, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_set_nested_shorter()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	const char* tnetstring = "27:5:outer,15:3:foo,6:foobar,}}";

	/* when changing the length of a nested key's value */
	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "outer");
	inner = new_nested_data_access(data_access, term);
	assert(set_and_check(inner, "foo", "bar", 3, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_set_multiple()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	const char* tnetstring = "36:3:foo,3:bar,5:outer,12:3:foo,3:bar,}}";

	/* when changing multiple values on different levels */
	data_access = new_data_access(tnetstring);
	assert(set_and_check(data_access, "foo", "foobar", 6, LTNS_STRING));

	term = get_term(data_access, "outer");
	inner = new_nested_data_access(data_access, term);
	assert(set_and_check(inner, "foo", "foobar", 6, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_set_multiple_scope()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;

	/* when changing multiple values on different levels while re-using scoped data_accesses */
	const char* tnetstring = "64:4:key1,3:bar,5:outer,26:4:key1,3:bar,4:key2,3:bar,}4:key2,3:bar,}";
	//const char* new_data = "76:4:key1,6:foobar,5:outer,32:4:key1,6:foobar,4:key2,6:foobar,}4:key2,6:foobar,}";
	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "outer");
	inner = new_nested_data_access(data_access, term);

	assert(set_and_check(inner, "key1", "foobar", 6, LTNS_STRING));
	assert(set_and_check(inner, "key2", "foobar", 6, LTNS_STRING));
	assert(set_and_check(data_access, "key1", "foobar", 6, LTNS_STRING));
	assert(set_and_check(data_access, "key2", "foobar", 6, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_set_multiple_scope_interleaved()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
//	char* long_str = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // 100 bytes
	char* long_str = "11111111112222222222333333333344444444445555555555"; // 50 bytes

	/* when changing multiple interleaved values on different levels while re-using scoped data_accesses */
	const char* tnetstring = "64:4:key1,3:bar,5:outer,26:4:key1,3:bar,4:key2,3:bar,}4:key2,3:bar,}";
	//const char* new_data = "76:4:key1,6:foobar,5:outer,32:4:key1,6:foobar,4:key2,6:foobar,}4:key2,6:foobar,}";
	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "outer");
	inner = new_nested_data_access(data_access, term);

	assert(set_and_check(data_access, "key1", "foobar", 6, LTNS_STRING));
	assert(set_and_check(inner, "key1", long_str, strlen(long_str), LTNS_STRING));
	assert(set_and_check(data_access, "key2", "foobar", 6, LTNS_STRING));
	assert(set_and_check(inner, "key2", "foobar", 6, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_set_invalidating_scope()
{
	LTNSDataAccess *data_access = NULL, *level1, *level2, *level3, *newlevel3;
	LTNSTerm *term = NULL;
	LTNSError error;

	/* when updating inner hashes so that references to old keys get invalid */
	const char* tnetstring = "51:6:level1,38:6:level2,25:6:level3,12:3:key,3:bar,}}}}" ;
	const char* tnet_newlevel3 = "31:9:newlevel3,15:3:key,6:foobar,}}";

	data_access = new_data_access(tnetstring);
	term = get_term(data_access, "level1");
	level1 = new_nested_data_access(data_access, term);
	assert(!LTNSTermDestroy(term));

	term = get_term(level1, "level2");
	level2 = new_nested_data_access(level1, term);
	assert(!LTNSTermDestroy(term));

	term = get_term(level2, "level3");
	level3 = new_nested_data_access(level2, term);
	assert(!LTNSTermDestroy(term));

	newlevel3 = new_data_access(tnet_newlevel3);
	assert(!LTNSDataAccessAsTerm(newlevel3, &term));
	assert(set_key(level1, "level2", term));
	LTNSTermDestroy(term);
	LTNSDataAccessDestroy(newlevel3);

	term = NULL;
	error = LTNSDataAccessGet(level3, "key", &term);
	assert(error == INVALID_CHILD);
	assert(term == NULL);
	error = LTNSDataAccessGet(level2, "level3", &term);
	assert(error == INVALID_CHILD);
	assert(term == NULL);
	/* NOTE: Dangling children must be manually destroyed */
	assert(!LTNSDataAccessDestroy(level2));

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_set_empty()
{
	LTNSDataAccess *data_access = NULL;
	const char* tnetstring = "0:}";

	/* when updating a non-existing key in an empty hash */
	data_access = new_data_access(tnetstring);
	assert(set_and_check(data_access, "foo", "bar", 3, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_set_non_empty()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	const char* tnetstring = "16:4:key1,6:value1,}";

	/* when updating a non-existing key to a non-empty hash */
	data_access = new_data_access(tnetstring);
	assert(set_and_check(data_access, "key2", "value2", 6, LTNS_STRING));
	term = get_term(data_access, "key1");
	assert(check_term(term, "value1", 6, LTNS_STRING));

	assert(!LTNSDataAccessDestroy(data_access));
	assert(!LTNSTermDestroy(term));
	return 1;
}

int test_set_nested()
{
	// TODO: implement
	return 1;
}

int test_set_known_null()
{
	// TODO: implement
	return 1;
}

int test_set_unknown_null()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	LTNSError error;
	const char* tnetstring = "0:}";

	/* when updating a key to null */
	data_access = new_data_access(tnetstring);
	LTNSDataAccessSet(data_access, "foo", NULL);
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(error == KEY_NOT_FOUND);
	assert(term == NULL);

	assert(!LTNSDataAccessDestroy(data_access));
	return 1;
}

int test_set_nested_null()
{
	// TODO: implement
	return 1;
}

