#include <ruby.h>

#include "LTNS.h"

int ltns_parse_string(const char* payload, size_t length, VALUE* out);
int ltns_parse_num(const char* payload, size_t length, VALUE* out);
int ltns_parse_float(const char* payload, size_t length, VALUE* out);
int ltns_parse_bool(const char* payload, size_t length, VALUE* out);
int ltns_parse_array(const char* payload, size_t length, VALUE* out);
int ltns_parse_nil(const char* payload, size_t length, VALUE* out);
int ltns_parse(const char* tnetstring, const char* end, VALUE* out);

extern VALUE eInvalidTNetString;

VALUE ltns_parse_ruby(VALUE module __attribute__ ((unused)), VALUE string)
{
	VALUE ret = Qnil;

	if (TYPE(string) != T_STRING)
	{
		VALUE rb_eArgumentError = rb_const_get(rb_cObject, rb_intern("ArgumentError"));
		rb_raise(rb_eArgumentError, "Invalid argument");
	}

	char* tnetstring = StringValueCStr(string);
	char* tnet_end = tnetstring + RSTRING_LEN(string);
	if (!ltns_parse(tnetstring, tnet_end, &ret))
	{
		rb_raise(eInvalidTNetString, "Invalid TNetstring");
	}
	return ret;
}

int ltns_parse(const char* tnetstring, const char* end, VALUE* out)
{
	if (!tnetstring)
		return FALSE;

	LTNSTerm *term = NULL;
	LTNSError error = LTNSTermCreateNested(&term, (char*)tnetstring, (char*)end);
	if (error)
	{
		LTNSTermDestroy(term);
		return FALSE;
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
		break;
	case LTNS_INTEGER:
		ret = ltns_parse_num(payload, payload_length, out);
		break;
	case LTNS_FLOAT:
		ret = ltns_parse_float(payload, payload_length, out);
		break;
	case LTNS_BOOLEAN:
		ret = ltns_parse_bool(payload, payload_length, out);
		break;
	case LTNS_LIST:
		ret = ltns_parse_array(payload, payload_length, out);
		break;
	case LTNS_DICTIONARY:
		ret = FALSE; // TODO: non-lazy parsing (ltns_parse_hash(payload, payload_length, out))
		break;
	case LTNS_NULL:
		ret = ltns_parse_nil(payload, payload_length, out);
		break;
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
	if (length != 4 && length != 5)
		return FALSE;

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

int ltns_parse_array(const char* payload, size_t payload_length, VALUE* out)
{
	size_t offset = 0;
	LTNSTerm *term = NULL;
	LTNSError error;
	size_t length;
	char *tnetstring;

	VALUE array = rb_ary_new();

	while (offset < payload_length)
	{
		error = LTNSTermCreateNested(&term, (char*)payload + offset, (char*)payload + payload_length);
		LTNSTermGetTNetstring(term, &tnetstring, &length);
		LTNSTermDestroy(term);
		if (error)
			return FALSE;

		VALUE element;
		int ok = ltns_parse(tnetstring, payload + payload_length, &element); 
		if (!ok)
			return FALSE;

		rb_ary_push(array, element);

		offset += length;
	}

	*out = array;

	return TRUE;
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
