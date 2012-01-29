#include "LTNSDataAccess.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN_HASH_LENGTH 3 // "0:}"
#define IS_ROOT(x) ((x)->offset == 0)
#define IS_CHILD(x) ((x)->offset > 0)
#define IS_ORPHAN(x) ((x)->parent == NULL)

static LTNSError LTNSDataAccessAdd(LTNSDataAccess* data_access, const char* key, LTNSTerm* term);
static LTNSError LTNSDataAccessUpdate(LTNSDataAccess* data_access, const char* key, LTNSTerm* old_term, LTNSTerm* new_term);
static LTNSError LTNSDataAccessShrink(LTNSDataAccess* data_access, char* tail_start, long length_delta);
static LTNSError LTNSDataAccessExpand(LTNSDataAccess* data_access, char* tail_start, long length_delta);
static LTNSError LTNSDataAccessReallocTNetstring(LTNSDataAccess *data_access, size_t new_length);


static LTNSError LTNSDataAccessCreatePrivate(LTNSDataAccess** data_access, const char* tnetstring, size_t length, int is_root);

static LTNSError LTNSDataAccessFindValueTerm(LTNSDataAccess* data_access, const char* key, LTNSTerm** term);
static LTNSError LTNSDataAccessAddChild(LTNSDataAccess* data_access, LTNSDataAccess* child);
static LTNSError LTNSDataAccessFindKeyPosition(LTNSDataAccess* data_access, const char* key, char** position, char** next);

static long LTNSDataAccessGetTotalLengthDelta(LTNSDataAccess* data_access, long length_delta);
static LTNSError LTNSDataAccessUpdatePrefixes(LTNSDataAccess* data_access, long length_delta, long prefix_length_deltas, LTNSDataAccess* root, long *total_length_delta);
static LTNSError LTNSDataAccessUpdateOffsets(LTNSDataAccess* data_access, long offset_delta, char* point_of_change);
static LTNSError LTNSDataAccessUpdateTNetstrings(LTNSDataAccess* data_access, LTNSDataAccess* root);

static LTNSChildNode* LTNSDataAccessFindChild(LTNSDataAccess* data_access, LTNSDataAccess* child);
static LTNSChildNode* LTNSDataAccessFindChildAt(LTNSDataAccess* data_access, char* position);
static int LTNSDataAccessIsChildValid(LTNSDataAccess* data_access);
static void LTNSDataAccessDeleteChildAt(LTNSDataAccess* data_access, char* position);

static LTNSError LTNSDataAccessTermOffset(LTNSDataAccess* data_access, LTNSTerm* term, size_t* offset);

struct _LTNSDataAccess
{
	unsigned int ref_count;
	char* tnetstring;
	size_t length; // NOTE: tnetstring + length points to the char *after* the type
	size_t offset; // NOTE: global offset, i.e. in relation to parent
	LTNSDataAccess* parent;
	LTNSChildNode* children;
};

/* The root (parent == NULL) owns the copy of the tnetstring */
static LTNSError LTNSDataAccessCreatePrivate(LTNSDataAccess** data_access,
		const char* tnetstring,
		size_t length,
		int is_root)
{
	if (!data_access)
		return INVALID_ARGUMENT;

	*data_access = NULL;
	if (length < MIN_HASH_LENGTH || !tnetstring)
		return INVALID_TNETSTRING;
	
	switch (tnetstring[length - 1])
	{
	case LTNS_DICTIONARY:
		break;
	case LTNS_STRING:
	case LTNS_INTEGER:
	case LTNS_BOOLEAN:
	case LTNS_LIST:
	case LTNS_NULL:
		return UNSUPPORTED_TOP_LEVEL_DATA_STRUCTURE;
	default:
		return INVALID_TNETSTRING;
	}

	*data_access = (LTNSDataAccess*)malloc(sizeof(LTNSDataAccess));
	if (!*data_access)
		return OUT_OF_MEMORY;

	if (is_root)
	{
		(*data_access)->tnetstring = (char*)malloc(sizeof(char) * length + 1);
		if (!(*data_access)->tnetstring)
			return OUT_OF_MEMORY;
		(*data_access)->tnetstring = memcpy((*data_access)->tnetstring, tnetstring, length);
		(*data_access)->tnetstring[length] = '\0';
	}
	else
	{
		(*data_access)->tnetstring = (char*)tnetstring;
	}

	(*data_access)->length = length;
	(*data_access)->offset = 0;
	(*data_access)->parent = NULL;
	(*data_access)->children = NULL;
	(*data_access)->ref_count = 1;

	return 0;
}

LTNSError LTNSDataAccessCreate(LTNSDataAccess** data_access, const char* tnetstring, size_t length)
{
	/* Check if tnetstring is valid */
	LTNSTerm *term = NULL;
	LTNSError error = LTNSTermCreateNested(&term, (char*)tnetstring, (char*)tnetstring + length);
	RETURN_VAL_IF(error);
	LTNSTermDestroy(term);

	return LTNSDataAccessCreatePrivate(data_access, tnetstring, length, TRUE);
}

LTNSError LTNSDataAccessCreateNested( LTNSDataAccess **child, LTNSDataAccess *parent, LTNSTerm *term)
{
	char *tnetstring = NULL;
	size_t length = 0, offset = 0;
	LTNSError error;

	if( !term || !child || !parent)
		return INVALID_ARGUMENT;

	LTNSType type;
	LTNSTermGetPayloadType( term, &type );
	if( type != LTNS_DICTIONARY )
		return INVALID_ARGUMENT;


	error = LTNSDataAccessTermOffset(parent, term, &offset);
	RETURN_VAL_IF( error );
	error = LTNSTermGetTNetstring(term, &tnetstring, &length);
	RETURN_VAL_IF( error );

	/* Don't create new child if it is already in the parent's child list */
	LTNSChildNode *node = LTNSDataAccessFindChildAt(parent, tnetstring);
	if (node)
	{
		*child = node->child;
		/* Increase the reference count if we are returning a cached child */
		(*child)->ref_count++;
		return 0;
	}

	error = LTNSDataAccessCreatePrivate(child, tnetstring, length, FALSE);
	RETURN_VAL_IF(error);

	(*child)->parent = parent;
	error = LTNSDataAccessAddChild(parent, *child);
	if (error)
	{
		LTNSDataAccessDestroy(*child);
		*child = NULL;
		return error;
	}

	(*child)->offset = offset;
	return 0;
}

LTNSError LTNSDataAccessDestroy(LTNSDataAccess* data_access)
{
	LTNSChildNode *node, *to_delete;

	if (!data_access)
		return INVALID_ARGUMENT;

	/* Only free if the number of references are 0 */
	data_access->ref_count--;
	if (data_access->ref_count > 0)
		return 0;

	if (data_access->children)
	{
		node = data_access->children;
		while (node)
		{
			/* Orphan the child */
			node->child->parent = NULL;
			to_delete = node;
			node = node->next;
			free(to_delete);
		}
	}

	/* If data_access is root (ie has no parent) then we free the tnetstring */
	if (IS_ROOT(data_access))
	{
		free(data_access->tnetstring);
	}
	else if (IS_CHILD(data_access) && !IS_ORPHAN(data_access))
	{
		LTNSDataAccessDeleteChildAt(data_access->parent, data_access->tnetstring);
	}

	free(data_access);

	return 0;
}

LTNSError LTNSDataAccessParent(LTNSDataAccess* data_access, LTNSDataAccess** parent)
{
	if (!data_access || !parent)
		return INVALID_ARGUMENT;

	*parent = data_access->parent;
	return 0;
}

LTNSError LTNSDataAccessChildren(LTNSDataAccess* data_access, LTNSChildNode** first_child)
{
	if (!data_access || !first_child)
		return INVALID_ARGUMENT;

	*first_child = data_access->children;
	return 0;
}

LTNSError LTNSDataAccessOffset(LTNSDataAccess* data_access, size_t* offset)
{
	if (!data_access || !offset)
		return INVALID_ARGUMENT;

	*offset = data_access->offset;
	return 0;
}

static LTNSError LTNSDataAccessTermOffset(LTNSDataAccess* data_access, LTNSTerm* term, size_t* offset)
{
	LTNSError error;
	char* nested_tnetstring = NULL;

	if (!data_access || !term || !offset)
		return INVALID_ARGUMENT;

	error = LTNSTermGetTNetstring(term, &nested_tnetstring, NULL);
	RETURN_VAL_IF(error);

	/* Check that the nested term is within this data_access */
	if (nested_tnetstring < data_access->tnetstring)
		return INVALID_ARGUMENT;
	if (nested_tnetstring >= (data_access->tnetstring + data_access->length))
		return INVALID_ARGUMENT;

	*offset = nested_tnetstring - (data_access->tnetstring - data_access->offset);
	return 0;
}

LTNSError LTNSDataAccessGet(LTNSDataAccess* data_access, const char* key, LTNSTerm** term)
{
	if (!term || !data_access)
		return INVALID_ARGUMENT;
	if (!key)
	{
		*term = NULL;
		return INVALID_ARGUMENT;
	}
	if (IS_CHILD(data_access) && !LTNSDataAccessIsChildValid(data_access))
		return INVALID_CHILD;

	*term = NULL;
	return LTNSDataAccessFindValueTerm(data_access, key, term);
}

LTNSError LTNSDataAccessSet(LTNSDataAccess* data_access, const char* key, LTNSTerm* term)
{
	LTNSError error = 0;
	LTNSTerm *old_term = NULL;

	if (!data_access || !key || !term)
		return INVALID_ARGUMENT;
	if (IS_CHILD(data_access) && !LTNSDataAccessIsChildValid(data_access))
		return INVALID_CHILD;

	char* tnetstring;
	size_t length = 0;
	LTNSTerm *copy = NULL;
	LTNSTermGetTNetstring(term, &tnetstring, &length);
	char tnet_copy[length+1];
	/* Check that the nested term is within this data_access (ie nested) */
	if ((tnetstring >= data_access->tnetstring)
		&& (tnetstring < (data_access->tnetstring + data_access->length)))
	{
		memcpy(tnet_copy, tnetstring, length);
		tnet_copy[length] = '\0';
		LTNSTermCreateFromTNestring(&copy, tnet_copy);
		term = copy;
	}

	/* Check if we are updating or adding */
	error = LTNSDataAccessGet(data_access, key, &old_term);
	if (!error && old_term)
	{
		error = LTNSDataAccessUpdate(data_access, key, old_term, term);
		LTNSTermDestroy(old_term);
		LTNSTermDestroy(copy);
		RETURN_VAL_IF(error);
	}
	else if (error == KEY_NOT_FOUND) // For add new
	{
		error = LTNSDataAccessAdd(data_access, key, term);
		LTNSTermDestroy(copy);
		RETURN_VAL_IF(error);
	}
	else
		LTNSTermDestroy(copy);
	return 0;
}

LTNSError LTNSDataAccessAsTerm(LTNSDataAccess* data_access, LTNSTerm** term)
{
	LTNSError error;
	if (!data_access || !term)
		return INVALID_ARGUMENT;
	if (IS_CHILD(data_access) && !LTNSDataAccessIsChildValid(data_access))
		return INVALID_CHILD;

	error = LTNSTermCreateNested(term, data_access->tnetstring, data_access->tnetstring + data_access->length);
	RETURN_VAL_IF(error)

	return 0;
}

LTNSError LTNSDataAccessRemove(LTNSDataAccess* data_access, const char* key)
{
	if (!data_access || !key)
		return INVALID_ARGUMENT;
	if (IS_CHILD(data_access) && !LTNSDataAccessIsChildValid(data_access))
		return INVALID_CHILD;

	/* Find the key position */
	char* key_position = NULL;
	char* value_position = NULL;
	LTNSError error = LTNSDataAccessFindKeyPosition(data_access, key, &key_position, &value_position);
	RETURN_VAL_IF(error);

	LTNSTerm *value_term = NULL;
	error = LTNSTermCreateNested(&value_term, value_position, data_access->tnetstring + data_access->length);
	RETURN_VAL_IF(error);

	/* Check if we are removing a child */
	LTNSType type = LTNS_UNDEFINED;
	error = LTNSTermGetPayloadType(value_term, &type);
	if (type == LTNS_DICTIONARY)
		LTNSDataAccessDeleteChildAt(data_access, value_position);

	/* Find length of value so we know where it ends */
	size_t value_length = 0;
	error = LTNSTermGetTNetstring(value_term, NULL, &value_length);
	LTNSTermDestroy(value_term);
	RETURN_VAL_IF(error);

	char* tail_start = value_position + value_length;
	long length_delta = key_position - tail_start;
	return LTNSDataAccessShrink(data_access, tail_start, length_delta);
}

static LTNSError LTNSDataAccessAdd(LTNSDataAccess* data_access, const char* key, LTNSTerm* value_term)
{
	/* Create term for new key */
	LTNSTerm *key_term = NULL;
	LTNSError error = LTNSTermCreate(&key_term, key, strlen(key), LTNS_STRING);
	RETURN_VAL_IF(error);
	char* key_tnetstring = NULL;
	size_t key_length = 0;
	LTNSTermGetTNetstring(key_term, &key_tnetstring, &key_length);

	/* Insert the key at the end */
	char* tail_start = data_access->tnetstring + data_access->length - 1;
	error = LTNSDataAccessExpand(data_access, tail_start, key_length);
	RETURN_VAL_IF(error);
	tail_start = data_access->tnetstring + data_access-> length - 1 - key_length;
	memcpy(tail_start, key_tnetstring, key_length);
	LTNSTermDestroy(key_term);

	/* Insert the value at the end */
	char* value_tnetstring = NULL;
	size_t value_length = 0;
	LTNSTermGetTNetstring(value_term, &value_tnetstring, &value_length);
	tail_start = data_access->tnetstring + data_access->length - 1;
	error = LTNSDataAccessExpand(data_access, tail_start, value_length);
	RETURN_VAL_IF(error);
	tail_start = data_access->tnetstring + data_access-> length - 1 - value_length;
	memcpy(tail_start, value_tnetstring, value_length);

	return 0;
}

static LTNSError LTNSDataAccessUpdate(LTNSDataAccess* data_access, const char* key, LTNSTerm* old_term, LTNSTerm* new_term)
{
	char* old_tnetstring = NULL;
	size_t old_length = 0;
	char* new_tnetstring = NULL;
	size_t new_length = 0;

	LTNSError error = LTNSTermGetTNetstring(old_term, &old_tnetstring, &old_length);
	RETURN_VAL_IF(error);
	error = LTNSTermGetTNetstring(new_term, &new_tnetstring, &new_length);
	RETURN_VAL_IF(error);
	long length_delta = new_length - old_length;

	/* Check if we are overwriting a child */
	LTNSType old_type = LTNS_UNDEFINED;
	error = LTNSTermGetPayloadType(old_term, &old_type);
	if (old_type == LTNS_DICTIONARY)
		LTNSDataAccessDeleteChildAt(data_access, old_tnetstring);

	if (length_delta == 0) // no length change, just update payload/type
	{
		// NOTE: does not support overlapping terms!
		memcpy(old_tnetstring, new_tnetstring, old_length);
		return 0;
	}
	else
	{
		char* tail_start = old_tnetstring + old_length;
		if (length_delta < 0)
		{
			error = LTNSDataAccessShrink(data_access, tail_start, length_delta);
			RETURN_VAL_IF(error);
		}
		else
		{
			error = LTNSDataAccessExpand(data_access, tail_start, length_delta);
			RETURN_VAL_IF(error);
		}

		/* NOTE: At this point the value term's prefix is
		 * incorrect and any children *after* the value term
		 * will have invalid offsets! */
		char* key_position = NULL;
		char* new_position = NULL;
		error = LTNSDataAccessFindKeyPosition(data_access, key, &key_position, &new_position);
		RETURN_VAL_IF(error);

		/* Write new term at new_position */
		memcpy(new_position, new_tnetstring, new_length);
	}

	return 0;
}

static LTNSError LTNSDataAccessShrink(LTNSDataAccess* data_access, char* tail_start, long length_delta)
{
	if (!data_access || !tail_start || length_delta >= 0)
		return INVALID_ARGUMENT;

	/* Update prefixes */
	LTNSDataAccess *root = LTNSDataAccessGetRoot(data_access);
	long total_length_delta = 0;
	LTNSError error = LTNSDataAccessUpdatePrefixes(data_access, length_delta, 0, root, &total_length_delta);
	RETURN_VAL_IF(error);

	/* Tail might have moved due to prefix length changes */
	tail_start += (total_length_delta - length_delta);
	size_t tail_offset = tail_start - root->tnetstring;

	/* Move tail */
	size_t tail_length = (root->tnetstring + root->length - length_delta + 1) - tail_start;
	memmove(tail_start + length_delta, tail_start, tail_length);

	/* Realloc root tnetstring */
	error = LTNSDataAccessReallocTNetstring(root, root->length + 1);
	RETURN_VAL_IF(error);

	/* tail_start may have moved after realloc */
	tail_start = root->tnetstring + tail_offset;

	/* Update offsets for every child after tail_start */
	return LTNSDataAccessUpdateOffsets(root, length_delta, tail_start);
}

static LTNSError LTNSDataAccessExpand(LTNSDataAccess* data_access, char* tail_start, long length_delta)
{
	if (!data_access || !tail_start || length_delta <= 0)
		return INVALID_ARGUMENT;

	LTNSDataAccess *root = LTNSDataAccessGetRoot(data_access);
	size_t tail_offset = tail_start - root->tnetstring;

	/* Realloc root tnetstring */
	long total_length_delta = LTNSDataAccessGetTotalLengthDelta(data_access, length_delta);
	LTNSError error = LTNSDataAccessReallocTNetstring(root, root->length + total_length_delta + 1);
	RETURN_VAL_IF(error);

	/* tail_start may have moved after realloc */
	tail_start = root->tnetstring + tail_offset;

	/* Update prefixes */
	error = LTNSDataAccessUpdatePrefixes(data_access, length_delta, 0, root, &total_length_delta);
	RETURN_VAL_IF(error);

	/* Tail might have moved due to prefix length changes */
	tail_start += (total_length_delta - length_delta);

	/* Move tail */
	size_t tail_length = (root->tnetstring + root->length - length_delta + 1) - tail_start;
	memmove(tail_start + length_delta, tail_start, tail_length);

	/* Update offsets for every child after tail_start */
	return LTNSDataAccessUpdateOffsets(root, length_delta, tail_start);
}

static LTNSError LTNSDataAccessReallocTNetstring(LTNSDataAccess *data_access, size_t new_length)
{
	if (!data_access || IS_CHILD(data_access))
		return INVALID_ARGUMENT;

	char* new_root_tnetstring = realloc(data_access->tnetstring,  new_length);
	if (!new_root_tnetstring)
		return OUT_OF_MEMORY;
	data_access->tnetstring = new_root_tnetstring;
	return LTNSDataAccessUpdateTNetstrings(data_access, data_access);
}

static LTNSError LTNSDataAccessAddChild(LTNSDataAccess* data_access, LTNSDataAccess* child)
{
	LTNSChildNode *last;

	if (!data_access->children)
	{
		data_access->children = (LTNSChildNode*)malloc(sizeof(LTNSChildNode));
		if (!data_access->children)
			return OUT_OF_MEMORY;
		data_access->children->child = child;
		data_access->children->next = NULL;

		return 0;
	}

	last = data_access->children;
	while (last->next)
		last = last->next;

	last->next = (LTNSChildNode*)malloc(sizeof(LTNSChildNode));
	if (!last->next)
		return OUT_OF_MEMORY;
	last->next->child = child;
	last->next->next = NULL;

	return 0;
}

static LTNSError LTNSDataAccessFindKeyPosition(LTNSDataAccess* data_access, const char* key, char** position, char** next)
{
	LTNSError error = 0;
	LTNSTerm* term;
	char* payload;
	size_t payload_len;
	size_t key_len = strlen(key);
	size_t term_len;
	char* tnetstring;
	char* end = data_access->tnetstring + data_access->length;


	/* Skip the tnetstring pointer ahead of the prefix to the payload */
	error = LTNSDataAccessAsTerm(data_access, &term);
	RETURN_VAL_IF(error);
	error = LTNSTermGetPayload(term, &tnetstring, NULL, NULL);
	RETURN_VAL_IF(LTNSTermDestroy(term));
	RETURN_VAL_IF(error);

	while (!error && tnetstring < end - 1)
	{
		error = LTNSTermCreateNested(&term, tnetstring, end);
		RETURN_VAL_IF(error);
		error = LTNSTermGetPayload(term, &payload, &payload_len, NULL);
		RETURN_VAL_IF(LTNSTermDestroy(term));
		RETURN_VAL_IF(error);

		/* Check the parsed key matches search key */
		if (key_len == payload_len && !memcmp(payload, key, key_len))
		{
			*position = tnetstring;
			*next = payload + payload_len + 1;
			return 0;
		}
		/* Skip key */
		tnetstring = payload + payload_len + 1;
		if (tnetstring > end)
			break;
		/* Get length of value term */
		error = LTNSTermCreateNested(&term, tnetstring, end);
		RETURN_VAL_IF(error);
		error = LTNSTermGetTNetstring(term, NULL, &term_len);
		RETURN_VAL_IF(LTNSTermDestroy(term));
		RETURN_VAL_IF(error);
		/* Skip key's value */
		tnetstring += term_len;
	}

	return KEY_NOT_FOUND;
}

static LTNSError LTNSDataAccessFindValueTerm(LTNSDataAccess* data_access, const char* key, LTNSTerm** term)
{
	LTNSError error;
	char* key_position = NULL;
	char* val_position = NULL;

	/* Find key */
	error = LTNSDataAccessFindKeyPosition(data_access, key, &key_position, &val_position);
	RETURN_VAL_IF(error);
	error = LTNSTermCreateNested(term, val_position, data_access->tnetstring + data_access->length);
	RETURN_VAL_IF(error);

	return 0;
}

LTNSDataAccess *LTNSDataAccessGetRoot( LTNSDataAccess* data_access )
{
	while(data_access && IS_CHILD(data_access))
		data_access = data_access->parent;

	return data_access;
}

static long LTNSDataAccessGetTotalLengthDelta(LTNSDataAccess* data_access, long length_delta)
{
	LTNSTerm *term = NULL;
	size_t payload_length = 0;
	char *payload;

	if (data_access)
	{
		/* NOTE: No error handling here =/ */
		LTNSDataAccessAsTerm(data_access, &term);
		LTNSTermGetPayload(term, &payload, &payload_length, NULL);
		size_t old_prefix_length = count_digits(payload_length);
		size_t new_prefix_length = count_digits(payload_length + length_delta);
		long prefix_length_delta = new_prefix_length - old_prefix_length;

		LTNSTermDestroy(term);

		return LTNSDataAccessGetTotalLengthDelta(data_access->parent, prefix_length_delta + length_delta);
	}

	return length_delta;

}

static LTNSError LTNSDataAccessUpdatePrefixes(LTNSDataAccess* data_access, long length_delta, long prefix_length_deltas, LTNSDataAccess* root, long *total_length_delta)
{
	if (!data_access || length_delta == 0)
		return INVALID_ARGUMENT;

	// Parse old prefix
	char *colon = NULL;
	size_t payload_length = (size_t)strtol(data_access->tnetstring, &colon, 10);
	if (!colon || *colon != ':' || colon == data_access->tnetstring)
		return INVALID_TNETSTRING;

	size_t old_prefix_length = count_digits(payload_length);
	size_t new_prefix_length = count_digits(payload_length + length_delta);
	long prefix_length_delta = new_prefix_length - old_prefix_length;

	// Shift all following data
	if (prefix_length_delta != 0)
	{
		// root->length may be inaccurate because of shifts in
		// previous recursions, so we need to add all previous shifts
		size_t shift_length = (root->tnetstring + root->length + prefix_length_deltas + 1) - colon;
		memmove(colon + prefix_length_delta, colon, shift_length);
	}

	// Write new prefix
	snprintf(data_access->tnetstring, new_prefix_length + 1, "%lu", payload_length + length_delta);
	*(colon + prefix_length_delta) = ':';

	// store new length in data access
	data_access->length += length_delta + prefix_length_delta;

	// Update child offsets/pointers for prefix length changes
	if (prefix_length_delta != 0)
	{
		LTNSError error = LTNSDataAccessUpdateOffsets(root, prefix_length_delta, data_access->tnetstring);
		RETURN_VAL_IF(error);
	}

	if (IS_CHILD(data_access))
		return LTNSDataAccessUpdatePrefixes(data_access->parent, length_delta + prefix_length_delta, prefix_length_deltas + prefix_length_delta, root, total_length_delta);
	else
	{
		*total_length_delta = length_delta + prefix_length_delta;
		return 0;
	}
}

static LTNSError LTNSDataAccessUpdateOffsets(LTNSDataAccess* data_access, long offset_delta, char* point_of_change)
{
	if (!data_access || offset_delta == 0)
		return INVALID_ARGUMENT;

	/* Only update children after point_of_change && never update root */
	if (data_access->tnetstring > point_of_change && IS_CHILD(data_access))
	{
		data_access->offset += offset_delta;
		data_access->tnetstring += offset_delta;
	}

	// Update child offsets
	LTNSChildNode *node = data_access->children;
	while (node)
	{
		LTNSError error = LTNSDataAccessUpdateOffsets(node->child, offset_delta, point_of_change);
		RETURN_VAL_IF(error);
		node = node->next;
	}

	return 0;
}

static LTNSError LTNSDataAccessUpdateTNetstrings(LTNSDataAccess* data_access, LTNSDataAccess* root)
{
	if (!data_access || !root)
		return INVALID_ARGUMENT;

	data_access->tnetstring = root->tnetstring + data_access->offset;

	// Update child offsets
	LTNSChildNode *node = data_access->children;
	while (node)
	{
		LTNSError error = LTNSDataAccessUpdateTNetstrings(node->child, root);
		RETURN_VAL_IF(error);
		node = node->next;
	}

	return 0;
}

static LTNSChildNode* LTNSDataAccessFindChild(LTNSDataAccess* data_access, LTNSDataAccess* child)
{
	LTNSChildNode *node = data_access->children;

	while (node)
	{
		if (node->child == child)
			return node;
		node = node->next;
	}

	return NULL;
}

static LTNSChildNode* LTNSDataAccessFindChildAt(LTNSDataAccess* data_access, char* position)
{
	LTNSChildNode *node = data_access->children;

	while (node)
	{
		if (node->child->tnetstring == position)
			return node;
		node = node->next;
	}

	return NULL;
}

static int LTNSDataAccessIsChildValid(LTNSDataAccess* data_access)
{
	if (!data_access)
		return FALSE;

	LTNSDataAccess* child = data_access;
	LTNSDataAccess* parent = data_access->parent;
	while (parent)
	{
		if (!LTNSDataAccessFindChild(parent, child))
			return FALSE;
		if (IS_ROOT(parent))
			return TRUE;
		child = parent;
		parent = child->parent;
	}

	return FALSE;
}

static void LTNSDataAccessDeleteChildAt(LTNSDataAccess* data_access, char* position)
{
	LTNSChildNode *prev = NULL;
	LTNSChildNode *node = data_access->children;
	while (node)
	{
		if (node->child->tnetstring == position)
		{
			/* Orphan the child by setting it's parent to NULL */
			node->child->parent = NULL;
			if (prev)
			{
				prev->next = node->next;
			}
			else
			{
				data_access->children = node->next;
			}
			free(node);
			break;
		}
		prev = node;
		node = node->next;
	}
}
