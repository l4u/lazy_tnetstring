#ifndef DATA_ACCESS_H
#define DATA_ACCESS_H

#include <stdlib.h>

struct _DataAccess;
typedef struct _DataAccess DataAccess;

#include "term.h"


struct Node;
typedef struct
{
	DataAccess* child;
	struct Node* next;
} Node;


typedef enum
{
	INVALID_TNETSTRING,
	UNSUPPORTED_TOP_LEVEL_DATA_STRUCTURE,
	INVALID_SCOPE
} Error;

Error data_access_create(DataAccess** data_access, const char* data);
Error data_access_create_with_parent(DataAccess** data_access, const char* data, DataAccess* parent);
Error data_access_create_with_scope(DataAccess** data_access, const char* data, DataAccess* parent, const char* scope);

DataAccess* data_access_parent(DataAccess* data_access);
Node* data_access_children(DataAccess* data_access);

size_t data_access_offset(DataAccess* data_access);

Error data_access_get(DataAccess* data_access, const char* key, Term** term);
Error data_access_set(DataAccess* data_access, const char* key, Term* term);

Term* data_access_as_term(DataAccess* data_access);

#endif
