// DUMMY IMPLEMENTATION FEEL FREE TO IMPLMENT ALL FUNCTIONS
#include <stdlib.h>
#include "LTNSTerm.h"

int LTNSCreateTerm( LTNSTerm **term, char *payload, size_t payload_length, LTNSType type )
{
	return 0;
}

int LTNSDestroyTerm( LTNSTerm *term )
{
	return 0;
}

int LTNSGetPayload( LTNSTerm *term, char **payload, size_t *length, LTNSType *type )
{
	return 0;
}

int LTNSGetPayloadLength( LTNSTerm *term, size_t *length )
{
	return 0;
}

int LTNSGetPayloadType( LTNSTerm *term, LTNSType *type )
{
	return LTNS_UNDEFINED;
}

