#ifndef __LTNSTERM_H___
#define __LTNSTERM_H___

typedef struct 
{
	size_t length;
	char* tnetstring;
	char* payload;
} LTNSTerm;

typedef enum
{
	TYPE_STRING = ',',
	TYPE_INTEGER = '#',
	TYPE_BOOLEAN = '!',
	TYPE_LIST = ']',
	TYPE_DICTIONARY = '}'
} LTNSType;


// Creates a new term
int LTNSTermCreate( LTNSTerm **term , const char* payload, size_t payload_length, LTNSType type );

// destroies the created term
int LTNSTermDestroy( LTNSTerm *term );

const char* LTNSTermPayload( LTNSTerm *term );
size_t LTNSTermPayloadLength( LTNSTerm *term );
LTNSType LTNSTermType( LTNSTerm *term );

#endif
