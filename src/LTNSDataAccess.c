#include "LTNSDataAccess.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN_HASH_LENGTH 3 // "0:}"

static LTNSError LTNSDataAccessCreatePrivate(LTNSDataAccess** data_access, const char* tnetstring, size_t length, int is_root);
static LTNSError LTNSDataAccessFindValueTerm(LTNSDataAccess* data_access, const char* key, LTNSTerm** term);
static LTNSError LTNSDataAccessAddChild(LTNSDataAccess* data_access, LTNSDataAccess* child);
static LTNSError LTNSDataAccessFindKeyPosition(LTNSDataAccess* data_access, const char* key, char** position, char** next);

static int LTNSDataAccessGetTotalLengthDelta(LTNSDataAccess* data_access, int length_delta);
static LTNSDataAccess *LTNSDataAccessGetRoot( LTNSDataAccess* data_access );
static LTNSError LTNSDataAccessUpdatePrefixes(LTNSDataAccess* data_access, int length_delta, LTNSDataAccess* root, int *total_length_delta);
static LTNSError LTNSDataAccessUpdateOffsets(LTNSDataAccess* data_access, int offset_delta, char* point_of_change);
static LTNSError LTNSDataAccessUpdateTNetstrings(LTNSDataAccess* data_access, LTNSDataAccess* root);

struct _LTNSDataAccess
{
	char* tnetstring;
	char* scope;
	size_t length;
	size_t offset; // NOTE: global offset, i.e. in relation to parent (not used/needed?)
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
	(*data_access)->scope = NULL;
	(*data_access)->parent = NULL;
	(*data_access)->children = NULL;

	return 0;
}

LTNSError LTNSDataAccessCreate(LTNSDataAccess** data_access, const char* tnetstring, size_t length)
{
	return LTNSDataAccessCreatePrivate(data_access, tnetstring, length, TRUE);
}

LTNSError LTNSDataAccessCreateWithParent(LTNSDataAccess** data_access,
		LTNSDataAccess* parent,
		size_t offset,
		size_t length)
{
	LTNSError error;
	if (!parent)
		return INVALID_ARGUMENT;

	if (offset >= parent->length)
		return INVALID_TNETSTRING;

	error = LTNSDataAccessCreatePrivate(data_access, (parent->tnetstring - parent->offset) + offset, length, FALSE);
	RETURN_VAL_IF(error);

	(*data_access)->parent = parent;
	error = LTNSDataAccessAddChild(parent, *data_access);
	if (error)
	{
		LTNSDataAccessDestroy(*data_access);
		*data_access = NULL;
		return error;
	}

	(*data_access)->offset = offset;

	return 0;
}
LTNSError LTNSDataAccessCreateWithScope(LTNSDataAccess** data_access,
		LTNSDataAccess* parent,
		size_t offset,
		size_t length,
		const char* scope)
{
	LTNSError error = LTNSDataAccessCreateWithParent(data_access, parent, offset, length);
	RETURN_VAL_IF(error);

	(*data_access)->scope = strdup(scope); // NOTE: scope must be null terminated!

	return 0;
}

LTNSError LTNSDataAccessDestroy(LTNSDataAccess* data_access)
{
	LTNSChildNode *child, *to_delete;

	if (data_access->scope)
		free(data_access->scope);
	if (data_access->children)
	{
		child = data_access->children;
		while (child)
		{
			to_delete = child;
			child = child->next;
			LTNSDataAccessDestroy(to_delete->child);
			free(to_delete);
		}
	}

	if (!data_access->parent)
		free(data_access->tnetstring);
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

LTNSError LTNSDataAccessTermOffset(LTNSDataAccess* data_access, LTNSTerm* term, size_t* offset)
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

LTNSError LTNSDataAccessScope(LTNSDataAccess* data_access, char** scope)
{
	if (!data_access || !scope)
		return INVALID_ARGUMENT;

	*scope = data_access->scope;
	return 0;
}

LTNSError LTNSDataAccessGet(LTNSDataAccess* data_access, const char* key, LTNSTerm** term)
{
	if (!term)
		return INVALID_ARGUMENT;
	if (!key)
	{
		*term = NULL;
		return KEY_NOT_FOUND;
	}

	*term = NULL;
	return LTNSDataAccessFindValueTerm(data_access, key, term);
}

LTNSError LTNSDataAccessSet(LTNSDataAccess* data_access, const char* key, LTNSTerm* term)
{
	LTNSError error = 0;
	LTNSTerm *old_term = NULL;
	char* old_tnetstring = NULL;
	size_t old_length = 0;
	char* new_tnetstring = NULL;
	size_t new_length = 0;

	if (!data_access || !key || !term)
		return INVALID_ARGUMENT;

	LTNSDataAccess *root = LTNSDataAccessGetRoot( data_access );

	error = LTNSDataAccessGet(data_access, key, &old_term);
	// For update
	if (!error && old_term)
	{
		error = LTNSTermGetTNetstring(old_term, &old_tnetstring, &old_length);
		RETURN_VAL_IF(error);
		error = LTNSTermGetTNetstring(term, &new_tnetstring, &new_length);
		RETURN_VAL_IF(error);
		LTNSTermDestroy(old_term);
		int length_delta = new_length - old_length;

		if (length_delta == 0) // no length change, just update payload/type
		{
			// NOTE: does not support overlapping terms!
			memcpy(old_tnetstring, new_tnetstring, old_length);
			return 0;
		}
		else if (length_delta < 0)
		{
			// Step 0 update prefixes and offsets
			size_t old_offset = data_access->offset;
			int total_length_delta = 0;
			error = LTNSDataAccessUpdatePrefixes(data_access, length_delta, root, &total_length_delta);
			RETURN_VAL_IF(error);

			// Step 1 memmove for shorter value
			size_t new_offset = data_access->offset;
			char* new_position = old_tnetstring + (new_offset - old_offset);
			char* tail = new_position + old_length;
			size_t tail_length = (root->tnetstring + root->length - total_length_delta + 1) - tail;
			memmove(tail + length_delta, tail, tail_length);

			// Step 2 realloc root
			char* new_root_tnetstring = realloc(root->tnetstring, root->length + 1);
			if (!new_root_tnetstring)
				return OUT_OF_MEMORY;
			root->tnetstring = new_root_tnetstring;
			error = LTNSDataAccessUpdateTNetstrings(root, root);

			/* NOTE: At this point the value term's prefix is
			 * incorrect and any children *after* the value term
			 * will have invalid offsets! */
			/* realloc may have moved our data so we need to update new_position */
			char* key_position = NULL;
			error = LTNSDataAccessFindKeyPosition(data_access, key, &key_position, &new_position);
			RETURN_VAL_IF(error);

			// Step 3 update offsets for length change
			error = LTNSDataAccessUpdateOffsets(root, length_delta, new_position);
			RETURN_VAL_IF(error);

			// Step 4 write new term at new_position
			memcpy(new_position, new_tnetstring, new_length);
		}
		else
		{
			int total_length_delta = LTNSDataAccessGetTotalLengthDelta(data_access, length_delta);
			// Step 0 realloc root
			char* new_root_tnetstring = realloc(root->tnetstring, root->length + total_length_delta + 1);
			if (!new_root_tnetstring)
				return OUT_OF_MEMORY;
			root->tnetstring = new_root_tnetstring;
			error = LTNSDataAccessUpdateTNetstrings(root, root);

			/* realloc may have moved our data so fetch old_tnetstring again */
			char* key_position = NULL;
			char* new_position = NULL;
			error = LTNSDataAccessFindKeyPosition(data_access, key, &key_position, &new_position);
			RETURN_VAL_IF(error);

			// Step 1 update prefixes and offsets
			error = LTNSDataAccessUpdatePrefixes(data_access, length_delta, root, &total_length_delta);
			RETURN_VAL_IF(error);

			// Step 2 memmove for longer value
			char* tail = new_position + old_length;
			size_t tail_length = (root->tnetstring + root->length - total_length_delta + 1) - tail;
			memmove(tail + length_delta, tail, tail_length);

			// Step 3 update offsets for length change
			error = LTNSDataAccessUpdateOffsets(root, length_delta, new_position);
			RETURN_VAL_IF(error);

			// Step 4 write new term at new_position
			memcpy(new_position, new_tnetstring, new_length);
		}

	}
	else if (error == KEY_NOT_FOUND) // For add new
	{
	}
	return 0;
}

LTNSError LTNSDataAccessAsTerm(LTNSDataAccess* data_access, LTNSTerm** term)
{
	LTNSError error;
	if (!data_access || !term)
		return INVALID_ARGUMENT;

	error = LTNSTermCreateNested(term, data_access->tnetstring);
	RETURN_VAL_IF(error)

	return 0;
}

/* static scope */

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
	const char* end = data_access->tnetstring + data_access->length - 1;


	/* Skip the tnetstring pointer ahead of the prefix to the payload */
	error = LTNSDataAccessAsTerm(data_access, &term);
	RETURN_VAL_IF(error);
	error = LTNSTermGetPayload(term, &tnetstring, NULL, NULL);
	RETURN_VAL_IF(LTNSTermDestroy(term));
	RETURN_VAL_IF(error);

	while (!error && tnetstring < end)
	{
		error = LTNSTermCreateNested(&term, tnetstring);
		RETURN_VAL_IF(error);
		error = LTNSTermGetPayload(term, &payload, &payload_len, NULL);
		RETURN_VAL_IF(LTNSTermDestroy(term));
		RETURN_VAL_IF(error);

		/* Check the parsed key matches search key */
		if (!memcmp(payload, key, MIN(key_len, payload_len)))
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
		error = LTNSTermCreateNested(&term, tnetstring);
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
	error = LTNSTermCreateNested(term, val_position);
	RETURN_VAL_IF(error);

	return 0;
}

static LTNSDataAccess *LTNSDataAccessGetRoot( LTNSDataAccess* data_access )
{
	while(data_access && data_access->parent)
		data_access = data_access->parent;

	return data_access;
}

static int LTNSDataAccessGetTotalLengthDelta(LTNSDataAccess* data_access, int length_delta)
{
	LTNSTerm *term = NULL;
	size_t payload_length = 0;
	char *payload;

	if (!data_access)
	{
		/* NOTE: No error handling here =/ */
		LTNSDataAccessAsTerm(data_access, &term);
		LTNSTermGetPayload(term, &payload, &payload_length, NULL);
		int old_prefix_length = count_digits(payload_length);
		int new_prefix_length = count_digits(payload_length + length_delta);
		int prefix_length_delta = new_prefix_length - old_prefix_length;

		LTNSTermDestroy(term);

		return LTNSDataAccessGetTotalLengthDelta(data_access->parent, prefix_length_delta + length_delta);
	}

	return length_delta;

}

static LTNSError LTNSDataAccessUpdatePrefixes(LTNSDataAccess* data_access, int length_delta, LTNSDataAccess* root, int *total_length_delta)
{
	char *tnetstring, *payload;

	if (!data_access || length_delta == 0)
		return INVALID_ARGUMENT;

	LTNSTerm *term = NULL;
	size_t payload_length = 0;
	LTNSError error = LTNSDataAccessAsTerm(data_access, &term);
	RETURN_VAL_IF(error);
	error = LTNSTermGetPayload(term, &payload, &payload_length, NULL);
	RETURN_VAL_IF(error);
	int old_prefix_length = count_digits(payload_length);
	int new_prefix_length = count_digits(payload_length + length_delta);
	int prefix_length_delta = new_prefix_length - old_prefix_length;

	// Shift all following data
	char* colon = payload - 1;
	if (prefix_length_delta != 0)
	{
		size_t shift_length = (root->tnetstring + root->length + 1) - colon;
		memmove(colon + prefix_length_delta, colon, shift_length);
	}

	// Write new prefix
	error = LTNSTermGetTNetstring(term, &tnetstring, NULL);
	RETURN_VAL_IF(error);
	LTNSTermDestroy(term);
	snprintf(tnetstring, new_prefix_length + 1, "%lu", payload_length + length_delta);
	*(colon + prefix_length_delta) = ':';

	// store new length in data access
	data_access->length += length_delta;

	// Update child offsets/pointers for prefix length changes
	if (prefix_length_delta != 0)
	{
		error = LTNSDataAccessUpdateOffsets(data_access, prefix_length_delta, data_access->tnetstring);
		RETURN_VAL_IF(error);
	}

	if (data_access->parent)
		return LTNSDataAccessUpdatePrefixes(data_access->parent, length_delta + prefix_length_delta, root, total_length_delta);
	else
	{
		*total_length_delta = length_delta + prefix_length_delta;
		return 0;
	}
}

static LTNSError LTNSDataAccessUpdateOffsets(LTNSDataAccess* data_access, int offset_delta, char* point_of_change)
{
	if (!data_access || offset_delta == 0)
		return INVALID_ARGUMENT;

	/* Only update children after point_of_change && never update root */
	if (data_access->tnetstring >= point_of_change && data_access->parent)
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
