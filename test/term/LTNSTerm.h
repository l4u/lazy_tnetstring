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
int LTNSGetValueLength( LTNSTerm *term, size_t* length );
LTNSType LTNSGetType( LTNSTerm *term );

// setter and getter for value data
int LTNSSetInteger( LTNSTerm *term, int value );
int LTNSGetInteger( LTNSTerm *term, int* value );

int LTNSSetString( LTNSTerm *term, char* value, size_t length );
int LTNSGetString( LTNSTerm *term, char* value, size_t* length );

int LTNSSetBoolean( LTNSTerm *term, int value );
int LTNSGetBoolean( LTNSTerm *term, int* value );

int LTNSSetNull( LTNSTerm *term );

int LTNSSetList( LTNSTerm *term, char* value, size_t length );
int LTNSGetList( LTNSTerm *term, char* value, size_t *length );
int LTNSSetListElement( LTNSTerm *term, int key, char* data, size_t length );
int LTNSGetListElement( LTNSTerm *term, int key, char* data, size_t *length );

int LTNSSetDictionary( LTNSTerm *term, char* value, size_t length );
int LTNSGetDictionary( LTNSTerm *term, char* value, size_t*length );
int LTNSSetDictionaryElement( LTNSTerm *term, char* key, char* data, size_t length );
int LTNSGetDictionaryElement( LTNSTerm *term, char* key, char* data, size_t *length );

#endif//__LTNSTERM_H___
