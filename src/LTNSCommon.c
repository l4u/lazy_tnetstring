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
		return TRUE;
	default:
		return FALSE;
	}
}

size_t count_digits( int number )
{
	return number == 0 ? 1 : (size_t)(floor(log10(number)) + 1);
}
