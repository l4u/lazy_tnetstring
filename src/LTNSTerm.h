#ifndef __LTNSTERM_H___
#define __LTNSTERM_H___

#include <stdlib.h>

#include "LTNSCommon.h"

struct _LTNSTerm;
typedef struct _LTNSTerm LTNSTerm;

LTNSError LTNSTermCreate( LTNSTerm **term, const char *payload, size_t payload_length, LTNSType type );

LTNSError LTNSTermCreateNested( LTNSTerm **term, char *tnetstring );

LTNSError LTNSTermDestroy( LTNSTerm *term );

LTNSError LTNSTermGetPayload( LTNSTerm *term, char **payload, size_t *length, LTNSType *type );

LTNSError LTNSTermGetPayloadLength( LTNSTerm *term, size_t *length );

LTNSError LTNSTermGetPayloadType( LTNSTerm *term, LTNSType *type );

LTNSError LTNSTermGetTNetstring( LTNSTerm *term, char** tnetstring, size_t* length );

LTNSError LTNSTermParse(LTNSTerm* term);

#endif//__LTNSTERM_H___
