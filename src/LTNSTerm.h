#ifndef __LTNSTERM_H___
#define __LTNSTERM_H___

#include <stdlib.h>

// independend of strinrepresentaion, this term uses a length
typedef struct 
{
	size_t raw_length;
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
	LTNS_DICTIONARY= '}',
} LTNSType;

int LTNSTermCreate( LTNSTerm **term, char *payload, size_t payload_length, LTNSType type );

int LTNSTermCreateNested( LTNSTerm **term, char *raw_data, size_t raw_length );

int LTNSTermDestroy( LTNSTerm *term );

int LTNSTermGetPayload( LTNSTerm *term, char **payload, size_t *length, LTNSType *type );

int LTNSTermGetPayloadLength( LTNSTerm *term, size_t *length );

int LTNSTermGetPayloadType( LTNSTerm *term, LTNSType *type );

LTNSType LTNSTermGetType( LTNSTerm *term );

// Utilities
int count_digits( int number );

#endif//__LTNSTERM_H___
