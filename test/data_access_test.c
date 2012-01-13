#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "lazy_tnetstring.h"

void test_data_access_create()
{
	DataAccess *data_access = NULL, *parent;
	int err;

	/* for non-tnetstring compliant data */
	err = data_access_create(&data_access, "12345");
	assert(err == INVALID_TNETSTRING);
	assert(data_access == NULL);

	/* for anything but a hash at the top level */
	err = data_access_create(&data_access, "1:1#");
	assert(err == UNSUPPORTED_TOP_LEVEL_DATA_STRUCTURE);
	assert(data_access == NULL);

	/* for an empty hash */
	err = data_access_create(&data_access, "0:}");
	assert(!err);
	assert(data_access);
	assert(data_access_offset(data_access) == 0);

	/* for a hash */
	err = data_access_create(&data_access, "32:3:key,5:value,7:another,5:value,}");
	assert(!err);
	assert(data_access);
	assert(data_access_offset(data_access) == 0);

	/* with parent */
	err = data_access_create(&parent, "0:}");
	assert(!err);
	err = data_access_create_with_parent(&data_access, "0:}", parent);
	assert(!err);
	assert(data_access);
	assert(data_access_offset(data_access) == 0);
	assert(data_access_parent(data_access) == parent);
	assert(data_access_children(data_access));
	assert(data_access_children(parent)->child == data_access);

	/* with scope */
	err = data_access_create_with_scope(&data_access, "0:}", NULL, "scope");
	assert(!err);
	assert(data_access);
	assert(!strcmp(data_access_offset(data_access), "scope"));
}

void test_data_access_get()
{
	DataAccess *data_access = NULL, *inner;
	Term *term = NULL;
	const char* key = "foo";
	int err;

	/* for an empty hash */
	err = data_access_create(&data_access, "0:}");
	assert(!err);
	err = data_access_get(data_access, key, &term);
	assert(term == NULL);

	/* for unknown keys */
	err = data_access_create(&data_access, "12:3:baz,3:bar,}");
	assert(!err);
	err = data_access_get(data_access, key, &term);
	assert(term == NULL);

	/* for unknown keys */
	err = data_access_create(&data_access, "12:3:foo,3:bar,}");
	assert(!err);
	err = data_access_get(data_access, key, &term);
	assert(term != NULL);
	assert(term_type(term) == TYPE_STRING);
	assert(!strncmp(term_payload(term), "bar", 3));

	/* for nested hash */
	err = data_access_create(&data_access, "28:5:outer,16:5:inner,5:value,}}");
	assert(!err);
	err = data_access_get(data_access, "outer", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&inner, term_payload(term), data_access);
	assert(inner != NULL);
	err = data_access_get(inner, "inner", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "value"));

	/* for nested hash with non-existing key */
	err = data_access_create(&data_access, "38:5:outer,16:5:inner,5:value,}4:user,0:}}");
	assert(!err);
	err = data_access_get(data_access, "outer", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&inner, term_payload(term), data_access);
	assert(inner != NULL);
	err = data_access_get(inner, "non-existing-key", &term);
	assert(term == NULL);
}

void test_data_access_set()
{
	DataAccess *data_access = NULL, *inner, *level1, *level2, *level3, *newlevel3;
	Term *term = NULL;
	const char* key = "foo";
	int err;

	/* for single value updates */

	/* whithout changing length */
	err = data_access_create(&data_access, "12:3:foo,3:bar,}");
	assert(!err);
	data_access_set(data_access, key, "3:baz,");
	err = data_access_get(data_access, key, &term);
	assert(term != NULL);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "baz"));

	/* when changing the length of top-level key */
	err = data_access_create(&data_access, "12:3:foo,3:bar,}");
	assert(!err);
	data_access_set(data_access, key, "6:foobar,");
	err = data_access_get(data_access, key, &term);
	assert(term != NULL);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));

	/* when changing the length of a nested key's value */
	err = data_access_create(&data_access, "24:5:outer,12:3:foo,3:bar,}}");
	assert(!err);
	err = data_access_get(data_access, "outer", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&inner, term_payload(term), data_access);
	assert(inner != NULL);
	data_access_set(inner, key, "6:foobar,");
	err = data_access_get(inner, key, &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));

	/* when changing a nested key's value without changing the length */
	err = data_access_create(&data_access, "24:5:outer,12:3:foo,3:bar,}}");
	assert(!err);
	err = data_access_get(data_access, "outer", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&inner, term_payload(term), data_access);
	assert(inner != NULL);
	data_access_set(inner, key, "3:baz,");
	err = data_access_get(inner, key, &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "baz"));

	/* when changing multiple values on different levels */
	err = data_access_create(&data_access, "36:3:foo,3:bar,5:outer,12:3:foo,3:bar,}}" );
	assert(!err);
	data_access_set(data_access, key, "6:foobar,");
	err = data_access_get(data_access, key, &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));
	err = data_access_get(data_access, "outer", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&inner, term_payload(term), data_access);
	assert(inner != NULL);
	data_access_set(inner, key, "6:foobar,");
	err = data_access_get(inner, key, &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));

	/* when changing multiple values on different levels while re-using scoped data_accesses */
	const char* data = "64:4:key1,3:bar,5:outer,26:4:key1,3:bar,4:key2,3:bar,}4:key2,3:bar,}";
	//const char* new_data = "76:4:key1,6:foobar,5:outer,32:4:key1,6:foobar,4:key2,6:foobar,}4:key2,6:foobar,}";
	err = data_access_create(&data_access, data);
	assert(!err);
	err = data_access_get(data_access, "outer", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&inner, term_payload(term), data_access);
	assert(inner != NULL);
	data_access_set(inner, "key1", "6:foobar,");
	data_access_set(inner, "key2", "6:foobar,");
	data_access_set(data_access, "key1", "6:foobar,");
	data_access_set(data_access, "key2", "6:foobar,");
	err = data_access_get(data_access, "key1", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));
	err = data_access_get(data_access, "key2", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));
	err = data_access_get(inner, "key1", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));
	err = data_access_get(inner, "key2", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));

	/* when changing multiple interleaved values on different levels while re-using scoped data_accesses */
	data = "64:4:key1,3:bar,5:outer,26:4:key1,3:bar,4:key2,3:bar,}4:key2,3:bar,}";
	//const char* new_data = "76:4:key1,6:foobar,5:outer,32:4:key1,6:foobar,4:key2,6:foobar,}4:key2,6:foobar,}";
	err = data_access_create(&data_access, data);
	assert(!err);
	err = data_access_get(data_access, "outer", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&inner, term_payload(term), data_access);
	assert(inner != NULL);
	data_access_set(data_access, "key1", "6:foobar,");
	data_access_set(inner, "key1", "6:foobar,");
	data_access_set(data_access, "key2", "6:foobar,");
	data_access_set(inner, "key2", "6:foobar,");
	err = data_access_get(data_access, "key1", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));
	err = data_access_get(data_access, "key2", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));
	err = data_access_get(inner, "key1", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));
	err = data_access_get(inner, "key2", &term);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "foobar"));

	/* when updating inner hashes so that references to old keys get invalid */
	data = "51:6:level1,38:6:level2,25:6:level3,12:3:key,3:bar,}}}}" ;
	err = data_access_create(&data_access, data);
	assert(!err);
	err = data_access_get(data_access, "level1", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&level1, term_payload(term), data_access);
	assert(level1 != NULL);
	err = data_access_get(level1, "level2", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&level2, term_payload(term), level1);
	assert(level2 != NULL);
	err = data_access_get(level2, "level3", &term);
	assert(term_type(term) == TYPE_DICTIONARY);
	err = data_access_create_with_parent(&level3, term_payload(term), level2);
	assert(level3 != NULL);
	err = data_access_create(&newlevel3, "31:9:newlevel3,15:3:key,6:foobar,}}");
	assert(!err);
	data_access_set(level1, "level2", data_access_as_term(newlevel3));
	err = data_access_get(level3, "key", &term);
	assert(err == INVALID_SCOPE);
	
	/* when updating a key to nil */
	err = data_access_create(&data_access, "0:}");
	assert(!err);
	data_access_set(data_access, key, NULL);
	err = data_access_get(data_access, key, &term);
	assert(!err);
	assert(term == NULL);

	/* when updating a non-existing key in an empty hash */
	err = data_access_create(&data_access, "0:}");
	assert(!err);
	data_access_set(data_access, key, "3:bar,");
	err = data_access_get(data_access, key, &term);
	assert(!err);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "bar"));

	/* when updating a non-existing key to a non-empty hash */
	err = data_access_create(&data_access, "16:4:key1,6:value1,}");
	data_access_set(data_access, "key2", "6:value2,");
	err = data_access_get(data_access, "key1", &term);
	assert(!err);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "value1"));
	err = data_access_get(data_access, "key2", &term);
	assert(!err);
	assert(term_type(term) == TYPE_STRING);
	assert(!strcmp(term_payload(term), "value2"));
}

int main(int argc, const char *argv[])
{
	test_data_access_create();
	test_data_access_get();
	test_data_access_set();
	return 0;
}
