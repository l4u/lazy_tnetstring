#ifndef __LTNSDATAACCESS_H__
#define __LTNSDATAACCESS_H__

#include "LTNSCommon.h"
#include "LTNSTerm.h"

struct _LTNSDataAccess;
typedef struct _LTNSDataAccess LTNSDataAccess;

typedef struct LTNSChildNode
{
	LTNSDataAccess* child;
	struct LTNSChildNode* next;
} LTNSChildNode;


LTNSError LTNSDataAccessCreate(LTNSDataAccess** data_access, const char* tnetstring, size_t length);
LTNSError LTNSDataAccessCreateNested(LTNSDataAccess** child, LTNSDataAccess* parent, LTNSTerm *term);

LTNSError LTNSDataAccessDestroy(LTNSDataAccess* data_access);

LTNSError LTNSDataAccessParent(LTNSDataAccess* data_access, LTNSDataAccess** parent);
LTNSError LTNSDataAccessChildren(LTNSDataAccess* data_access, LTNSChildNode** first_child);

LTNSError LTNSDataAccessOffset(LTNSDataAccess* data_access, size_t* offset);

LTNSError LTNSDataAccessGet(LTNSDataAccess* data_access, const char* key, LTNSTerm** term);
LTNSError LTNSDataAccessSet(LTNSDataAccess* data_access, const char* key, LTNSTerm* term);

LTNSError LTNSDataAccessAsTerm(LTNSDataAccess* data_access, LTNSTerm** term);

LTNSDataAccess *LTNSDataAccessGetRoot( LTNSDataAccess* data_access );

int LTNSDataAccessIsChildCached(LTNSDataAccess* data_access, LTNSTerm* term);
#endif
