#ifndef __LTNS_COMMON_H__
#define __LTNS_COMMON_H__

#include <stdlib.h>
#include <math.h>

#define FALSE 0
#define TRUE (!FALSE)
#define MAX_PREFIX_LENGTH 10
#define MIN(a,b) ((a) < (b) ? a : b)
#define RETURN_VAL_IF(a) if (a) return a;

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

typedef enum
{
	INVALID_TNETSTRING = 1,
	UNSUPPORTED_TOP_LEVEL_DATA_STRUCTURE,
	INVALID_SCOPE,
	OUT_OF_MEMORY,
	INVALID_ARGUMENT,
	KEY_NOT_FOUND	
} LTNSError;

int LTNSTypeIsValid( char type );
size_t count_digits( int number );

#endif
