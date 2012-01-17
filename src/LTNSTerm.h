#ifndef __LTNSTERM_H___
#define __LTNSTERM_H___

#include <stdlib.h>

// independend of strinrepresentaion, this term uses a length
typedef struct 
{
	int raw_length;
	char *raw_data;
} LTNSTerm;

typedef enum 
{
	LTNS_UNDEFINED = 0,
	LTNS_INTEGER   = '#',
	LTNS_STRING    = ',',
	LTNS_BOOLEAN   = '!',
	LTNS_NULL      = '~',
	LTNS_LIST      = ']',
	LTNS_DICTIONARY= '}'
} LTNSType;

int LTNSCreateTerm( LTNSTerm **term, char *payload, size_t payload_length, LTNSType type );

int LTNSDestroyTerm( LTNSTerm *term );

int LTNSGetPayload( LTNSTerm *term, char **payload, size_t *length, LTNSType *type );

int LTNSGetPayloadLength( LTNSTerm *term, size_t *length );

int LTNSGetPayloadType( LTNSTerm *term, LTNSType *type );

LTNSType LTNSGetType( LTNSTerm *term );

#endif//__LTNSTERM_H___
