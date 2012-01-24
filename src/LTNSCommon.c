#include "LTNSCommon.h"

int LTNSTypeIsValid( char type )
{
	switch( type )
	{
	case LTNS_INTEGER   : 
	case LTNS_STRING    : 
	case LTNS_BOOLEAN   : 
	case LTNS_NULL      : 
	case LTNS_LIST      : 
	case LTNS_DICTIONARY:
	case LTNS_FLOAT     :
		return TRUE;
	default:
		return FALSE;
	}
}

size_t count_digits( unsigned long long int number )
{
	return number == 0 ? 1 : (size_t)(floorl(log10l(number)) + 1);
}
