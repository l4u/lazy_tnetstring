#ifndef __LTNSDATAACCESS_H__
#define __LTNSDATAACCESS_H__

#include <stdlib.h>

struct _LTNSDataAccess;
typedef struct _LTNSDataAccess LTNSDataAccess;

#include "LTNSTerm.h"


typedef struct LTNSChildNode
{
	LTNSDataAccess* child;
	struct LTNSChildNode* next;
} LTNSChildNode;


typedef enum
{
	INVALID_TNETSTRING = 1,
	UNSUPPORTED_TOP_LEVEL_DATA_STRUCTURE,
	INVALID_SCOPE,
	OUT_OF_MEMORY,
	INVALID_ARGUMENT,
	KEY_NOT_FOUND	
} LTNSError;

LTNSError LTNSDataAccessCreate(LTNSDataAccess** data_access, const char* tnetstring, size_t length);
LTNSError LTNSDataAccessCreateWithParent(LTNSDataAccess** data_access, LTNSDataAccess* parent, size_t offset, size_t length);
LTNSError LTNSDataAccessCreateWithScope(LTNSDataAccess** data_access, LTNSDataAccess* parent, size_t offset, size_t length, const char* scope);

LTNSError LTNSDataAccessDestroy(LTNSDataAccess* data_access);

LTNSError LTNSDataAccessParent(LTNSDataAccess* data_access, LTNSDataAccess** parent);
LTNSError LTNSDataAccessChildren(LTNSDataAccess* data_access, LTNSChildNode** first_child);

LTNSError LTNSDataAccessOffset(LTNSDataAccess* data_access, size_t* offset);
LTNSError LTNSDataAccessScope(LTNSDataAccess* data_access, char** scope);

LTNSError LTNSDataAccessGet(LTNSDataAccess* data_access, const char* key, LTNSTerm** term);
LTNSError LTNSDataAccessSet(LTNSDataAccess* data_access, const char* key, LTNSTerm* term);

LTNSError LTNSDataAccessAsTerm(LTNSDataAccess* data_access, LTNSTerm** term);

#endif
