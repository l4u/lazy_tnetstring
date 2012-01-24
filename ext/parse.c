#include <ruby.h>

#include "LTNS.h"

int ltns_parse_string(const char* payload, size_t length, VALUE* out);
int ltns_parse_num(const char* payload, size_t length, VALUE* out);
int ltns_parse_float(const char* payload, size_t length, VALUE* out);
int ltns_parse_bool(const char* payload, size_t length, VALUE* out);
int ltns_parse_array(const char* payload, size_t length, VALUE* out);
int ltns_parse_hash(const char* payload, size_t length, VALUE* out);
int ltns_parse_nil(const char* payload, size_t length, VALUE* out);

VALUE ltns_parse_ruby(VALUE string)
{
	VALUE ret = Qnil;
	if (TYPE(string) != T_STRING)
	{
		VALUE rb_eArgumentError = rb_const_get(rb_cObject, rb_intern("ArgumentError"));
		rb_raise(rb_eArgumentError, "Invalid argument");
	}

	char* tnetstring = StringValueCStr(string);
	if (!ltns_parse(tnetstring, &ret))
	{
		free(tnetstring);
		VALUE rb_eArgumentError = rb_const_get(rb_cObject, rb_intern("ArgumentError"));
		rb_raise(rb_eArgumentError, "Invalid TNetstring");
	}
	free(tnetstring);
	return ret;
}

int ltns_parse(const char* tnetstring, VALUE* out)
{
	if (!tnetstring)
		return Qnil;

	LTNSTerm *term = NULL;
	LTNSError error = LTNSTermCreateNested(&term, tnetstring);
	if (error)
	{
		LTNSTermDestroy(term);
		return Qnil;
	}
	char* payload;
	size_t payload_length;
	LTNSType type;
	LTNSTermGetPayload(term, &payload, &payload_length, &type);
	LTNSTermDestroy(term);

	int ret = 0;
	switch (type)
	{
	case LTNS_STRING:
		ret = ltns_parse_string(payload, payload_length, out);
	case LTNS_INTEGER:
		ret = ltns_parse_num(payload, payload_length, out);
	case LTNS_FLOAT:
		ret = ltns_parse_float(payload, payload_length, out);
	case LTNS_BOOLEAN:
		ret = ltns_parse_bool(payload, payload_length, out);
	case LTNS_LIST:
		ret = ltns_parse_array(payload, payload_length, out);
	case LTNS_DICTIONARY:
		ret = ltns_parse_hash(payload, payload_length, out);
	case LTNS_NULL:
		ret = ltns_parse_nil(payload, payload_length, out);
	default:
		ret = FALSE;
	}
	
	return ret;
}

int ltns_parse_string(const char* payload, size_t length, VALUE* out)
{
	*out = rb_str_new(payload, length);
	return TRUE;
}

int ltns_parse_num(const char* payload, size_t length, VALUE* out)
{
	char *end;
	long parsed_val = strtol(payload, &end, 10);
	if (end != (payload + length) || *end != LTNS_INTEGER)
		return FALSE;
	*out = LL2NUM(parsed_val);
	return TRUE;
}

int ltns_parse_float(const char* payload, size_t length, VALUE* out)
{
	char *end;
	double parsed_val = strtod(payload, &end);
	if (end != (payload + length) || *end != LTNS_FLOAT)
		return FALSE;
	*out = DBL2NUM(parsed_val);
	return TRUE;
}

int ltns_parse_bool(const char* payload, size_t length, VALUE* out)
{
	int ret = TRUE;
	if (strncmp(payload, "true", 4) == 0)
	{
		*out = Qtrue;
	}
	else if (strncmp(payload, "false", 5) == 0)
	{
		*out = Qfalse;
	}
	else
	{
		ret = FALSE;
	}
	return ret;
}

int ltns_parse_array(const char* payload, size_t length, VALUE* out)
{
}

int ltns_parse_hash(const char* payload, size_t length, VALUE* out)
{
	/* Return a DataAccess */
}

int ltns_parse_nil(const char* payload, size_t length, VALUE* out)
{
	if (length == 0 && *payload == LTNS_NULL)
	{
		*out = Qnil;
		return TRUE;
	}
	return FALSE;
}
