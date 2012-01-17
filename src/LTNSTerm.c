#include <stdio.h>
#include <string.h>

#include "LTNSTerm.h"
#include "LTNSDataAccess.h"

// recursively count numbers, avoiding math.h implementation
int count_digits( int number )
{
	if( number < 10 && number >= 0 )
	{
		return 1;
	}
	else if(number >= 10 )
	{
		return count_digits( number / 10 ) + 1;
	}
	else 
	{	// negative number given
		return 0;
	}
}

static int is_valid_type( int type )
{
	switch( type )
	{
        	case LTNS_INTEGER   : 
        	case LTNS_STRING    : 
        	case LTNS_BOOLEAN   : 
        	case LTNS_NULL      : 
        	case LTNS_LIST      : 
        	case LTNS_DICTIONARY:
			return 1;
		default:
			return 0;
	}
}

int LTNSTermCreate( LTNSTerm **term, char *payload, size_t payload_length, LTNSType type )
{
	if( 	term == NULL || 
		(payload == NULL && type != LTNS_NULL) ||
		(payload == NULL && payload_length != 0 ) ||
		!is_valid_type(type) )
	{
		return INVALID_ARGUMENT;
	}

	*term = (LTNSTerm*)calloc(1, sizeof(LTNSTerm));

	// length calculation:          payload length + : + payload...     + type
	size_t length = count_digits( payload_length ) + 1 + payload_length + 1;
	(*term)->raw_length = length;
	(*term)->raw_data = (char*) calloc(1, length);
	int printed_chars = snprintf((*term)->raw_data, length, "%lu:", payload_length );
	memcpy((*term)->raw_data + printed_chars, payload, payload_length);

	// set type
	(*term)->raw_data[length -1] = (char)(type);
	
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
	if( term == NULL || length == NULL || term->raw_length < 3)
	{
		return INVALID_ARGUMENT;
	}

	size_t converted_length = 0;
	char* current_char = term->raw_data;

	while( *current_char != ':' && current_char - term->raw_data < 10 )
	{
		converted_length *= 10;
		converted_length += *current_char - '0';
		current_char ++;
	}

	if( *current_char == ':' )
	{
		*length = converted_length;
		return 0;
	}
	return 0;
}

int LTNSTermGetPayloadType( LTNSTerm *term, LTNSType *type )
{
	char retrieved_type = term->raw_data[term->raw_length-1];
	if( is_valid_type( retrieved_type ) ) 
	{
		*type = (LTNSType)retrieved_type;
		return 0;
	}
	else
	{
		return INVALID_ARGUMENT;
	}
}

