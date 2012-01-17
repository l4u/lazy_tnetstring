#include <LTNSDataAccess.h>
#include <string.h>

#define MIN_HASH_LENGTH 3 // "0:}"
#define MAX_PREFIX_LENGTH 10
#define MIN(a,b) ((a) < (b) ? a : b)

static LTNSError LTNSDataAccessFindValueTerm(LTNSDataAccess* data_access, const char* key, LTNSTerm** term);
static LTNSError LTNSDataAccessParseAt(LTNSDataAccess* data_access, size_t offset, char** payload, size_t* payload_length);
static LTNSError LTNSDataAccessAddChild(LTNSDataAccess* data_access, LTNSDataAccess* child);

struct _LTNSDataAccess
{
	char* tnetstring;
	char* scope;
	size_t length;
	size_t offset; // NOTE: global offset, i.e. in relation to parent
	LTNSDataAccess* parent;
	LTNSChildNode* children;
};

LTNSError LTNSDataAccessCreate(LTNSDataAccess** data_access, const char* tnetstring, size_t length)
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

	*data_access = malloc(sizeof(LTNSDataAccess));
	if (!*data_access)
		return OUT_OF_MEMORY;

	(*data_access)->tnetstring = malloc(sizeof(char) * length);
	if (!(*data_access)->tnetstring)
		return OUT_OF_MEMORY;
	(*data_access)->tnetstring = memcpy((*data_access)->tnetstring, tnetstring, length);

	(*data_access)->length = length;
	(*data_access)->scope = NULL;
	(*data_access)->parent = NULL;
	(*data_access)->children = NULL;

	return 0;
}

LTNSError LTNSDataAccessCreateWithParent(LTNSDataAccess** data_access,
		const char* tnetstring,
		size_t length,
		LTNSDataAccess* parent,
		size_t offset)
{
	LTNSError error;
	if (offset >= length)
		return INVALID_TNETSTRING;

	error = LTNSDataAccessCreate(data_access, tnetstring + offset, length - offset);
	if (error)
		return error;

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
		const char* tnetstring,
		size_t length,
		LTNSDataAccess* parent,
		size_t offset,
		const char* scope)
{
	LTNSError error = LTNSDataAccessCreateWithParent(data_access, tnetstring, length, parent, offset);
	if (error)
		return error;

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
			free(to_delete->child);
			free(to_delete);
		}
	}

	free(data_access->tnetstring);
	free(data_access);

	return 0;
}

LTNSDataAccess* LTNSDataAccessParent(LTNSDataAccess* data_access)
{
	if (!data_access)
		return NULL;

	return data_access->parent;
}

LTNSChildNode* LTNSDataAccessChildren(LTNSDataAccess* data_access)
{
	if (!data_access)
		return NULL;

	return data_access->children;
}

size_t LTNSDataAccessOffset(LTNSDataAccess* data_access)
{
	if (!data_access)
		return 0;

	return data_access->offset;
}

const char* LTNSDataAccessScope(LTNSDataAccess* data_access)
{
	if (!data_access)
		return NULL;

	return data_access->scope;
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

LTNSTerm* LTNSDataAccessAsTerm(LTNSDataAccess* data_access)
{
	LTNSError error;
	char* payload;
	size_t payload_len;
	LTNSTerm *term = NULL;
	
	if (!data_access)
		return NULL;

	error = LTNSDataAccessParseAt(data_access, 0, &payload, &payload_len);
	if (error)
		return NULL;
	error = LTNSCreateTerm(&term, payload, payload_len, payload[payload_len + 1]);
	if (error)
		return NULL;

	return term;
}

/* static scope */

static LTNSError LTNSDataAccessAddChild(LTNSDataAccess* data_access, LTNSDataAccess* child)
{
	LTNSChildNode *last;

	if (!data_access->children)
	{
		data_access->children = malloc(sizeof(LTNSChildNode));
		data_access->children->child = NULL;
		data_access->children->next = NULL;
	}

	last = data_access->children;
	while (last->next)
		last = last->next;

	last->next = malloc(sizeof(LTNSChildNode));
	last->next->child = child;
	last->next->next = NULL;

	return 0;
}

static LTNSError LTNSDataAccessFindValueTerm(LTNSDataAccess* data_access, const char* key, LTNSTerm** term)
{
	LTNSError error = 0;
	char* payload;
	size_t payload_len;
	size_t offset = 0;
	size_t key_len = strlen(key);

	while (!error)
	{
		error = LTNSDataAccessParseAt(data_access, offset, &payload, &payload_len);
		if (error)
			break;

		offset += (payload + payload_len) - data_access->tnetstring + 2;

		if (!bcmp(payload, key, MIN(key_len, payload_len)))
		{
			error = LTNSDataAccessParseAt(data_access, offset, &payload, &payload_len);
			if (error)
				return error;
			error = LTNSCreateTerm(term, payload, payload_len, payload[payload_len + 1]);
			if (error)
				return error;

			return 0;
		}
	}

	return KEY_NOT_FOUND;
}

static LTNSError LTNSDataAccessParseAt(LTNSDataAccess* data_access, size_t offset, char** payload, size_t* payload_length)
{
	char* colon;
	*payload = NULL;
	*payload_length = 0;
	if (offset >= data_access->length)
		return INVALID_TNETSTRING;

	*payload_length = (size_t)strtol(data_access->tnetstring + offset, &colon, 10);
	if (!colon || *colon != ':' || *colon == '\0' || colon > (data_access->tnetstring + offset + MAX_PREFIX_LENGTH))
	{
		*payload_length = 0;
		return INVALID_TNETSTRING;
	}

	*payload = colon + 1;

	return 0;
}
