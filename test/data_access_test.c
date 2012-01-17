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

// TODO: strcmp to bcmp, payload not null terminated! also check that LTNSTermGetPayloadLength is correct!

test_case tests[] = 
{
	/* create */
	{test_create_non_compliant, "create with non-tnetstring compliant data"},
	{test_create_non_hash_toplevel, "create with anything but a hash at the top level"},
	{test_create_empty, "create with an empty hash"},
	{test_create, "create with a hash"},
	{test_create_with_parent, "create with parent"},
	{test_create_with_scope, "create with scope"},
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
	
int test_create_non_compliant()
{
	LTNSDataAccess *data_access = NULL;
	LTNSError error;
	const char* tnetstring = "12345";

	/* for non-tnetstring compliant data */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(error == INVALID_TNETSTRING);
	assert(data_access == NULL);

	return 0;
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

	return 0;
}

int test_create_empty()
{
	LTNSDataAccess *data_access = NULL;
	LTNSError error;
	const char* tnetstring = "0:}";

	/* for an empty hash */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	assert(data_access);
	size_t offset = 0;
	assert(LTNSDataAccessOffset(data_access, &offset) );
	assert(offset == 0);
	
	return 0;
}

int test_create()
{
	LTNSDataAccess *data_access = NULL;
	LTNSError error;
	const char* tnetstring = "32:3:key,5:value,7:another,5:value,}";

	/* for a hash */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	assert(data_access);
	size_t offset = 0;
	assert(LTNSDataAccessOffset(data_access, &offset));
	assert( offset == 0 );

	return 0;
}

int test_create_with_parent()
{
	LTNSDataAccess *data_access = NULL, *parent;
	LTNSError error;
	const char* tnetstring = "0:}";

	/* with parent */
	error = LTNSDataAccessCreate(&parent, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessCreateWithParent(&data_access, tnetstring, strlen(tnetstring), parent, 0);
	assert(!error);
	assert(data_access);
	size_t offset = 0;
	assert(LTNSDataAccessOffset(data_access, &offset));
	assert( offset == 0 );
	
	LTNSDataAccess *returned_parent = NULL;
	assert(LTNSDataAccessParent(data_access, &returned_parent) );
	assert(returned_parent == parent);
	
	LTNSChildNode *child = NULL;
	assert(LTNSDataAccessChildren(data_access, &child));
	assert(child);

	assert(LTNSDataAccessChildren(parent, &child) );
	assert( child->child == data_access );

	return 0;
}

int test_create_with_scope()
{
	LTNSDataAccess *data_access = NULL;
	LTNSError error;
	const char* tnetstring = "0:}";

	/* with scope */
	error = LTNSDataAccessCreateWithScope(&data_access, tnetstring, strlen(tnetstring), NULL, 0, "scope");
	assert(!error);
	assert(data_access);
	char* scope = NULL;
	assert(LTNSDataAccessScope(data_access, &scope));
	assert(!strcmp(scope, "scope"));
 

	return 0;
}


int test_get_empty()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "0:}";

	/* for an empty hash */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(term == NULL);
}

int test_get_unknown()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "12:3:baz,3:bar,}";

	/* for unknown keys */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(term == NULL);

	return 0;
}

int test_get_known()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "12:3:foo,3:bar,}";

	/* for known keys */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(term != NULL);
	LTNSType type = LTNS_UNDEFINED;
	assert(LTNSTermGetPayloadType(term, &type)); 
	assert(type == LTNS_STRING);

	char* payload = NULL;
	size_t length = 0;
	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(!strcmp(payload, "bar"));

	return 0;
}

int test_get_nested()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "28:5:outer,16:5:inner,5:value,}}";

	/* for nested hash */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "outer", &term);

	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	size_t length = 0;
	assert ( LTNSTermGetPayload( term, &payload, &length, &type ) );
	assert(type == LTNS_DICTIONARY);

	error = LTNSDataAccessCreateWithParent(&inner, tnetstring, strlen(tnetstring), data_access, 0);
	assert(inner != NULL);
	error = LTNSDataAccessGet(inner, "inner", &term);

	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(length == 5);
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "value"));

	return 0;
}

int test_get_unknown_nested()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "38:5:outer,16:5:inner,5:value,}4:user,0:}}";

	/* for nested hash with non-existing key */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "outer", &term);
	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	size_t length = 0;
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_DICTIONARY);
	error = LTNSDataAccessCreateWithParent(&inner, payload, length, data_access, 0);
	assert(inner != NULL);
	error = LTNSDataAccessGet(inner, "non-existing-key", &term);
	assert(term == NULL);

	return 0;
}

int test_set_same_length()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	
	int error;
	const char* tnetstring = "12:3:foo,3:bar,}";

	/* whithout changing length */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	LTNSTermCreate(&term, "baz", 3, LTNS_STRING);
	LTNSDataAccessSet(data_access, "foo", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(term != NULL);
	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	assert(LTNSTermGetPayload(term, &payload, NULL, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "baz"));

	return 0;
}

int test_set_longer()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "12:3:foo,3:bar,}";

	/* when changing the length of top-level key */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	LTNSTermCreate(&term, "foobar", 6, LTNS_STRING);
	LTNSDataAccessSet(data_access, "foo", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(term != NULL);
	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	assert(LTNSTermGetPayload(term, &payload, NULL, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	return 0;
}

int test_set_shorter()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "12:3:foo,6:foobar,}";

	/* when changing the length of top-level key */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	LTNSTermCreate(&term, "bar", 3, LTNS_STRING);
	LTNSDataAccessSet(data_access, "foo", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(term != NULL);
	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	assert(LTNSTermGetPayload(term, &payload, NULL, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "bar"));

	return 0;
}


int test_set_nested_same_length()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "24:5:outer,12:3:foo,3:bar,}}";

	/* when changing a nested key's value without changing the length */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "outer", &term);
	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	size_t length = 0;
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_DICTIONARY);
	error = LTNSDataAccessCreateWithParent(&inner, payload, length, data_access, 0);
	assert(inner != NULL);
	LTNSTermCreate(&term, "baz", 3, LTNS_STRING);
	LTNSDataAccessSet(inner, "foo", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(inner, "foo", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "baz"));

	return 0;
}

int test_set_nested_longer()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "24:5:outer,12:3:foo,3:bar,}}";

	/* when changing the length of a nested key's value */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "outer", &term);
	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	size_t length = 0;
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_DICTIONARY);
	error = LTNSDataAccessCreateWithParent(&inner, payload, length, data_access, 0);
	assert(inner != NULL);
	LTNSTermCreate(&term, "foobar", 6, LTNS_STRING);
	LTNSDataAccessSet(inner, "foo", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(inner, "foo", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	return 0;
}

int test_set_nested_shorter()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "24:5:outer,12:3:foo,6:foobar,}}";

	/* when changing the length of a nested key's value */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "outer", &term);
	LTNSType type = LTNS_UNDEFINED;
	size_t length = 0;
	char *payload = NULL;
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_DICTIONARY);
	error = LTNSDataAccessCreateWithParent(&inner, payload, length, data_access, 0);
	assert(inner != NULL);
	LTNSTermCreate(&term, "bar", 3, LTNS_STRING);
	LTNSDataAccessSet(inner, "foo", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(inner, "foo", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "bar"));

	return 0;
}

int test_set_multiple()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "36:3:foo,3:bar,5:outer,12:3:foo,3:bar,}}";

	/* when changing multiple values on different levels */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	LTNSTermCreate(&term, "foobar", 6, LTNS_STRING);
	LTNSDataAccessSet(data_access, "foo", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(data_access, "foo", &term);
	LTNSType type = LTNS_UNDEFINED;
	size_t length = 0;
	char *payload = NULL;
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	error = LTNSDataAccessGet(data_access, "outer", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_DICTIONARY);

	error = LTNSDataAccessCreateWithParent(&inner, payload, length, data_access, 0);
	assert(inner != NULL);
	LTNSTermCreate(&term, "foobar", 6, LTNS_STRING);
	LTNSDataAccessSet(inner, "foo", term);
	LTNSTermDestroy(term);

	term = NULL;
	error = LTNSDataAccessGet(inner, "foo", &term);
	assert(LTNSTermGetPayload( term, &payload, &length, &type ) );
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	return 0;
}

int test_set_multiple_scope()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	int error;

	/* when changing multiple values on different levels while re-using scoped data_accesses */
	const char* tnetstring = "64:4:key1,3:bar,5:outer,26:4:key1,3:bar,4:key2,3:bar,}4:key2,3:bar,}";
	//const char* new_data = "76:4:key1,6:foobar,5:outer,32:4:key1,6:foobar,4:key2,6:foobar,}4:key2,6:foobar,}";
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "outer", &term);
	char *payload = NULL;
	size_t length = 0;
	LTNSType type = LTNS_UNDEFINED;
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_DICTIONARY);
	error = LTNSDataAccessCreateWithParent(&inner, payload, length, data_access, 0);
	assert(inner != NULL);

	LTNSTermCreate(&term, "foobar", 6, LTNS_STRING);
	LTNSDataAccessSet(inner, "key1", term);
	LTNSDataAccessSet(inner, "key2", term);
	LTNSDataAccessSet(data_access, "key1", term);
	LTNSDataAccessSet(data_access, "key2", term);
	LTNSTermDestroy(term);
	term = NULL;

	error = LTNSDataAccessGet(data_access, "key1", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	error = LTNSDataAccessGet(data_access, "key2", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	error = LTNSDataAccessGet(inner, "key1", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	error = LTNSDataAccessGet(inner, "key2", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	return 0;
}

int test_set_multiple_scope_interleaved()
{
	LTNSDataAccess *data_access = NULL, *inner;
	LTNSTerm *term = NULL;
	int error;

	/* when changing multiple interleaved values on different levels while re-using scoped data_accesses */
	const char* tnetstring = "64:4:key1,3:bar,5:outer,26:4:key1,3:bar,4:key2,3:bar,}4:key2,3:bar,}";
	//const char* new_data = "76:4:key1,6:foobar,5:outer,32:4:key1,6:foobar,4:key2,6:foobar,}4:key2,6:foobar,}";
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "outer", &term);

	LTNSType type = LTNS_UNDEFINED;
	size_t length = 0;
	char* payload = NULL;
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_DICTIONARY);
	
	error = LTNSDataAccessCreateWithParent(&inner, payload, length, data_access, 0);
	assert(inner != NULL);
	LTNSTermCreate(&term, "foobar", 6, LTNS_STRING);
	LTNSDataAccessSet(data_access, "key1", term);
	LTNSDataAccessSet(inner, "key1", term);
	LTNSDataAccessSet(data_access, "key2", term);
	LTNSDataAccessSet(inner, "key2", term);
	LTNSTermDestroy(term);
	term = NULL;

	error = LTNSDataAccessGet(data_access, "key1", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type)); 
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	error = LTNSDataAccessGet(data_access, "key2", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	error = LTNSDataAccessGet(inner, "key1", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	error = LTNSDataAccessGet(inner, "key2", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "foobar"));

	return 0;
}

int test_set_invalidating_scope()
{
	LTNSDataAccess *data_access = NULL, *level1, *level2, *level3, *newlevel3;
	LTNSTerm *term = NULL;
	int error;

	/* when updating inner hashes so that references to old keys get invalid */
	const char* tnetstring = "51:6:level1,38:6:level2,25:6:level3,12:3:key,3:bar,}}}}" ;
	const char* tnet_newlevel3 = "31:9:newlevel3,15:3:key,6:foobar,}}";

	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	error = LTNSDataAccessGet(data_access, "level1", &term);

	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	size_t length = 0;
	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(type == LTNS_DICTIONARY);
	error = LTNSDataAccessCreateWithParent(&level1, payload, length, data_access, 0);
	assert(level1 != NULL);
	error = LTNSDataAccessGet(level1, "level2", &term);

	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(type == LTNS_DICTIONARY);
	error = LTNSDataAccessCreateWithParent(&level2, payload, length, level1, 0);
	assert(level2 != NULL);

	error = LTNSDataAccessGet(level2, "level3", &term);
	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(type == LTNS_DICTIONARY);
	error = LTNSDataAccessCreateWithParent(&level3, payload, length, level2, 0);
	assert(level3 != NULL);
	
	error = LTNSDataAccessCreate(&newlevel3, tnet_newlevel3, strlen(tnet_newlevel3)); 
	assert(!error);
	assert(LTNSDataAccessAsTerm(newlevel3, &term));
	LTNSDataAccessSet(level1, "level2", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(level3, "key", &term);
	assert(error == INVALID_SCOPE);

	return 0;
}

int test_set_empty()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "0:}";

	/* when updating a non-existing key in an empty hash */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	LTNSTermCreate(&term, "bar", 3, LTNS_STRING);
	LTNSDataAccessSet(data_access, "foo", term);
	LTNSTermDestroy(term);
	term = NULL;
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(!error);
	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	size_t length = 0;
	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "bar"));

	return 0;
}

int test_set_non_empty()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "16:4:key1,6:value1,}";

	/* when updating a non-existing key to a non-empty hash */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	LTNSTermCreate(&term, "value2", 6, LTNS_STRING);
	LTNSDataAccessSet(data_access, "key2", term);
	LTNSTermDestroy(term);
	term = NULL;

	error = LTNSDataAccessGet(data_access, "key1", &term);
	assert(!error);
	LTNSType type = LTNS_UNDEFINED;
	char* payload = NULL;
	size_t length = 0;
	assert(LTNSTermGetPayload(term, &payload, &length, &type));
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "value1"));

	error = LTNSDataAccessGet(data_access, "key2", &term);
	assert(!error);

	assert(LTNSTermGetPayload(term, &payload, &length, &type ));
	assert(type == LTNS_STRING);
	assert(!strcmp(payload, "value2"));

	return 0;
}

int test_set_nested()
{
	// TODO: implement
	return 0;
}

int test_set_known_null()
{
	// TODO: implement
	return 0;
}

int test_set_unknown_null()
{
	LTNSDataAccess *data_access = NULL;
	LTNSTerm *term = NULL;
	int error;
	const char* tnetstring = "0:}";

	/* when updating a key to null */
	error = LTNSDataAccessCreate(&data_access, tnetstring, strlen(tnetstring));
	assert(!error);
	LTNSDataAccessSet(data_access, "foo", NULL);
	error = LTNSDataAccessGet(data_access, "foo", &term);
	assert(!error);
	assert(term == NULL);
}

int test_set_nested_null()
{
	// TODO: implement
	return 0;
}

