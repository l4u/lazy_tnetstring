#include <stdio.h>
#include <math.h>
#include <string.h>

#include "LTNSTerm.h"

struct _LTNSTerm
{
	size_t length;
	char *tnetstring;
	char *payload;
	size_t payload_length;
	char is_nested;
};

LTNSError LTNSTermCreate( LTNSTerm **term, char *payload, size_t payload_length, LTNSType type )
{
	if (term == NULL)
		return INVALID_ARGUMENT;

	if (	(payload == NULL && type != LTNS_NULL) ||
		(payload == NULL && payload_length != 0 ) ||
		!LTNSTypeIsValid(type) )
	{
		return INVALID_TNETSTRING;
	}

	*term = (LTNSTerm*)calloc(1, sizeof(LTNSTerm));
	if (!*term)
		return OUT_OF_MEMORY;

	(*term)->is_nested = FALSE;

	size_t prefix_length = count_digits(payload_length);
	// length calculation:  prefix length + : + payload...     + type
	size_t length =		prefix_length + 1 + payload_length + 1;
	(*term)->length = length;
	(*term)->tnetstring = (char*) calloc(length, sizeof(char));
	int printed_chars = snprintf((*term)->tnetstring, length, "%lu:", payload_length );
	memcpy((*term)->tnetstring + printed_chars, payload, payload_length);

	(*term)->payload = (*term)->tnetstring + prefix_length + 1;
	(*term)->payload_length = payload_length;

	// set type
	(*term)->tnetstring[length -1] = (char)(type);
	
	return 0;
}

LTNSError LTNSTermCreateNested( LTNSTerm **term, char *tnetstring )
{
	LTNSError error;

	if (!term)
		return INVALID_ARGUMENT;
	if (!tnetstring)
		return INVALID_TNETSTRING;

	*term = (LTNSTerm*)calloc(1, sizeof(LTNSTerm));
	if (!*term)
		return OUT_OF_MEMORY;

	// NOTE: Nested terms does not copy the tnetstring and should *not*
	// free it when destroyed!
	(*term)->is_nested = TRUE;
	(*term)->tnetstring = tnetstring;
	// Parse the payload, length etc...
	error = LTNSTermParse(*term);
	if (error)
	{
		free(*term);
		*term = NULL;
	}

	return error;
}

LTNSError LTNSTermDestroy( LTNSTerm *term )
{
	if (!term)
		return INVALID_ARGUMENT;

	if (!term->is_nested)
		free(term->tnetstring);

	free(term);

	return 0;
}

LTNSError LTNSTermGetPayload( LTNSTerm *term, char **payload, size_t *length, LTNSType *type )
{
	if (!term || !payload)
		return INVALID_ARGUMENT;

	*payload = term->payload;
	if (length)
		*length = term->payload_length;
	if (type)
		*type = term->tnetstring[term->length - 1];

	return 0;
}

LTNSError LTNSTermGetPayloadLength( LTNSTerm *term, size_t *length )
{
	if (!term || !length)
		return INVALID_ARGUMENT;

	*length = term->payload_length;

	return 0;
}


LTNSError LTNSTermGetPayloadType( LTNSTerm *term, LTNSType *type )
{
	char retrieved_type = LTNS_UNDEFINED;
	if (!term || !type)
		return INVALID_ARGUMENT;

	retrieved_type = term->tnetstring[term->length - 1];
	if( LTNSTypeIsValid( retrieved_type ) ) 
	{
		*type = (LTNSType)retrieved_type;
		return 0;
	}
	else
	{
		return INVALID_ARGUMENT;
	}
}

LTNSError LTNSTermGetTNetstring( LTNSTerm *term, char** tnetstring, size_t* length )
{
	if (!term)
		return INVALID_ARGUMENT;

	if (tnetstring)
		*tnetstring = term->tnetstring;
	if (length)
		*length = term->length;

	return 0;
}

LTNSError LTNSTermParse(LTNSTerm* term)
{
	char* colon;
	size_t prefix = 0;

	if (!term)
		return INVALID_ARGUMENT;

	prefix = (size_t)strtol(term->tnetstring, &colon, 10);
	/* No number found */
	if (colon == term->tnetstring)
		return INVALID_TNETSTRING;
	/* No colon found */
	if (*colon != ':' || *colon == '\0')
		return INVALID_TNETSTRING;
	/* Prefix longer than specification max length */
	if (colon > (term->tnetstring + MAX_PREFIX_LENGTH))
		return INVALID_TNETSTRING;

	/* Total term length is:     prefix length + COLON + payload + TYPE */
	term->length = (colon - term->tnetstring) + 1     + prefix  + 1;
	term->payload_length = prefix;
	/* Pointer to the begining of the payload string */
	term->payload = colon + 1;

	/* Check type */
	if (!LTNSTypeIsValid(term->payload[term->payload_length]))
		return INVALID_TNETSTRING;

	return 0;
}

