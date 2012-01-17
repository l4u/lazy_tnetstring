// DUMMY IMPLEMENTATION FEEL FREE TO IMPLMENT ALL FUNCTIONS
#include <stdlib.h>
#include "LTNSTerm.h"

int LTNSTermCreate( LTNSTerm **term, char *payload, size_t payload_length, LTNSType type )
{
	return 0;
}

int LTNSTermCreateNested( LTNSTerm **term, char *raw_data, size_t raw_length )
{
	return 0;
}

int LTNSTermDestroy( LTNSTerm *term )
{
	return 0;
}

int LTNSTermGetPayload( LTNSTerm *term, char **payload, size_t *length, LTNSType *type )
{
	return 0;
}

int LTNSTermGetPayloadLength( LTNSTerm *term, size_t *length )
{
	return 0;
}

int LTNSTermGetPayloadType( LTNSTerm *term, LTNSType *type )
{
	return LTNS_UNDEFINED;
}

