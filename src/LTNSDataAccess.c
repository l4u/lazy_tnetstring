#include "LTNSDataAccess.h"
#include <string.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE (!FALSE)
#define MIN_HASH_LENGTH 3 // "0:}"
#define MAX_PREFIX_LENGTH 10
#define MIN(a,b) ((a) < (b) ? a : b)
#define RETURN_VAL_IF(a) if (a) return a;

static LTNSError LTNSDataAccessCreatePrivate(LTNSDataAccess** data_access, const char* tnetstring, size_t length, int is_root);
static LTNSError LTNSDataAccessFindValueTerm(LTNSDataAccess* data_access, const char* key, LTNSTerm** term);
static LTNSError LTNSDataAccessAddChild(LTNSDataAccess* data_access, LTNSDataAccess* child);
static LTNSError LTNSDataAccessParseFirstTerm(const char* tnetstring, char** payload, size_t* payload_length, size_t* term_length);
static LTNSError LTNSDataAccessFindKeyPosition(LTNSDataAccess* data_access, const char* key, char** position, char** next);

struct _LTNSDataAccess
{
	char* tnetstring;
	char* scope;
	size_t length;
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
	if (length < MIN_HASH_LENGTH)
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

	if (offset >= length)
		return INVALID_TNETSTRING;

	error = LTNSDataAccessCreatePrivate(data_access, parent->tnetstring + offset, length, FALSE);
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
	if (!data_access || !term || !offset)
		return INVALID_ARGUMENT;
	if (term->raw_data < data_access->tnetstring)
		return INVALID_ARGUMENT;
	if (term->raw_data >= (data_access->tnetstring + data_access->length))
		return INVALID_ARGUMENT;

	*offset = term->raw_data - data_access->tnetstring;
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

	return LTNSDataAccessFindValueTerm(data_access, key, term);
}

LTNSError LTNSDataAccessSet(LTNSDataAccess* data_access, const char* key, LTNSTerm* term)
{
}

LTNSError LTNSDataAccessAsTerm(LTNSDataAccess* data_access, LTNSTerm** term)
{
	LTNSError error;
	if (!data_access || !term)
		return INVALID_ARGUMENT;

	error = LTNSTermCreateNested(term, data_access->tnetstring, data_access->length);
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
	char* payload;
	size_t payload_len;
	size_t key_len = strlen(key);
	size_t term_len;
	char* tnetstring;
	const char* end = data_access->tnetstring + data_access->length;

	/* Skip the tnetstring pointer ahead of the prefix to the payload */
	error = LTNSDataAccessParseFirstTerm(data_access->tnetstring, &tnetstring, NULL, NULL);

	while (!error && tnetstring <= end)
	{
		error = LTNSDataAccessParseFirstTerm(tnetstring, &payload, &payload_len, &term_len);
		RETURN_VAL_IF(error);

		/* Check the parsed key matches search key */
		if (!bcmp(payload, key, MIN(key_len, payload_len)))
		{
			*position = tnetstring;
			*next = tnetstring + term_len;
			return 0;
		}
		/* Skip key */
		tnetstring += term_len;
		if (tnetstring > end)
			break;
		/* If the parse does not match skip over it's value */
		error = LTNSDataAccessParseFirstTerm(tnetstring, NULL, NULL, &term_len);
		tnetstring += term_len;
	}

	return KEY_NOT_FOUND;
}

static LTNSError LTNSDataAccessFindValueTerm(LTNSDataAccess* data_access, const char* key, LTNSTerm** term)
{
	LTNSError error;
	size_t term_len = 0;
	char* key_position = NULL;
	char* val_position = NULL;

	/* Find key */
	error = LTNSDataAccessFindKeyPosition(data_access, key, &key_position, &val_position);
	RETURN_VAL_IF(error);
	/* Parse the next term (the key's value) */
	error = LTNSDataAccessParseFirstTerm(val_position, NULL, NULL, &term_len);
	RETURN_VAL_IF(error);
	error = LTNSTermCreateNested(term, val_position, term_len);
	RETURN_VAL_IF(error);

	return 0;
}

// FIXME: User LTNSTermCreateNested instead?
static LTNSError LTNSDataAccessParseFirstTerm(const char* tnetstring,
		char** payload,
		size_t* payload_length,
		size_t* term_length)
{
	char* colon;
	size_t prefix = 0;

	if (payload)
		*payload = NULL;

	prefix = (size_t)strtol(tnetstring, &colon, 10);
	if (colon == tnetstring)
		return INVALID_TNETSTRING;
	if (*colon != ':' || *colon == '\0')
		return INVALID_TNETSTRING;
	if (colon > (tnetstring + MAX_PREFIX_LENGTH))
		return INVALID_TNETSTRING;

	/* The length of the prefix + COLON + payload + TYPE */
	if (term_length)
		*term_length = (colon - tnetstring) + prefix + 2;
	if (payload_length)
		*payload_length = prefix;
	/* Pointer to the begining of the payload string */
	if (payload)
		*payload = colon + 1;

	return 0;
}
