#ifndef __LTNSTERM_H___
#define __LTNSTERM_H___

#include <stdlib.h>

// independend of strinrepresentaion, this term uses a length
typedef struct 
{
	int length;
	char* raw_data;
} LTNSTerm;

typedef enum 
{
	LTNS_UNDEFINED = 0,
	LTNS_INTEGER = 1,
	LTNS_STRING,
	LTNS_BOOLEAN,
	LTNS_NULL,
	LTNS_LIST,
	LTNS_DICTIONARY
} LTNSType;

// Creates a new term
int LTNSCreateTerm( LTNSTerm **term );

// destroies the created term
int LTNSDestroyTerm( LTNSTerm *term );

// setter and getter for raw data
int LTNSSetData( LTNSTerm *term, char* data, size_t length );
int LTNSGetData( LTNSTerm *term, char* data, size_t *length );
// TODO: implement payload length and term length functions
int LTNSGetValueLength( LTNSTerm *term, size_t* length );
LTNSType LTNSGetType( LTNSTerm *term );

// setter and getter for value data

#endif//__LTNSTERM_H___
