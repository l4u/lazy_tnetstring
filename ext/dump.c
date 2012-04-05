#include <st.h>

#include "LTNS.h"

#include "dump.h"
#include "data_access.h"

#ifndef FLOAT_DECIMAL_PRECISION
#define FLOAT_DECIMAL_PRECISION 3
#endif

typedef struct
{
	char *payload;
	size_t length;
} LTNSPayloadInfo;


static int ltns_dump_key_value(VALUE key, VALUE value, VALUE in);


VALUE ltns_dump(VALUE module __attribute__ ((unused)), VALUE val)
{
	VALUE rb_eArgumentError, ret;

	switch (TYPE(val))
	{
	case T_NIL:
		ret = ltns_dump_nil(val);
		break;
	case T_STRING:
		ret = ltns_dump_string(val);
		break;
	case T_FLOAT:
		ret = ltns_dump_by_to_s(val, LTNS_FLOAT);
		break;
	case T_HASH:
		ret = ltns_dump_hash(val);
		break;
	case T_ARRAY:
		ret = ltns_dump_array(val);
		break;
	case T_BIGNUM:
	case T_FIXNUM:
		ret = ltns_dump_by_to_s(val, LTNS_INTEGER);
		break;
	case T_FALSE:
	case T_TRUE:
		ret = ltns_dump_bool(val);
		break;
	case T_SYMBOL:
		ret = ltns_dump_string(rb_sym_to_s(val));
		break;
	case T_DATA:
		if (RDATA(val)->dfree == (RUBY_DATA_FUNC)ltns_da_free)
			ret = ltns_da_get_tnetstring(val);
		else
			ret = Qnil;
		break;
	default:
		ret = Qnil;
	}

	if (ret == Qnil)
	{
		rb_eArgumentError = rb_const_get(rb_cObject, rb_intern("ArgumentError"));
		rb_raise(rb_eArgumentError, "Invalid type");
	}

	return ret;
}

VALUE ltns_dump_by_to_s(VALUE val, LTNSType type)
{
	VALUE str = rb_funcall(val, rb_intern("to_s"), 0);

	LTNSTerm *term = NULL;
	LTNSError error = LTNSTermCreate(&term, RSTRING_PTR(str), RSTRING_LEN(str), type);
	if (error)
		return Qnil;
	char* tnetstring = NULL;
	size_t length;
	error = LTNSTermGetTNetstring(term, &tnetstring, &length);
	if (error)
		return Qnil;
	VALUE ret = rb_str_new(tnetstring, length);
	LTNSTermDestroy(term);

	return ret;
}

VALUE ltns_dump_string(VALUE val)
{
	LTNSTerm *term = NULL;
	LTNSError error = LTNSTermCreate(&term, RSTRING_PTR(val), RSTRING_LEN(val), LTNS_STRING);
	if (error)
		return Qnil;
	char* tnetstring = NULL;
	size_t length;
	error = LTNSTermGetTNetstring(term, &tnetstring, &length);
	if (error)
		return Qnil;
	VALUE ret = rb_str_new(tnetstring, length);
	LTNSTermDestroy(term);
	return ret;
}

VALUE ltns_dump_bool(VALUE val)
{
	LTNSTerm *term = NULL;
	LTNSError error;
	if (TYPE(val) == T_TRUE)
		error = LTNSTermCreate(&term, "true", 4, LTNS_BOOLEAN);
	else
		error = LTNSTermCreate(&term, "false", 5, LTNS_BOOLEAN);

	if (error)
		return Qnil;

	char* tnetstring = NULL;
	size_t length;
	error = LTNSTermGetTNetstring(term, &tnetstring, &length);
	if (error)
		return Qnil;
	VALUE ret = rb_str_new(tnetstring, length);

	LTNSTermDestroy(term);
	
	return ret;
}

VALUE ltns_dump_nil(VALUE val __attribute__ ((unused)))
{
	const char *nil = "0:~";
	return rb_str_new( nil, strlen(nil));
}

VALUE ltns_dump_array(VALUE val)
{
	size_t length = RARRAY_LEN(val);
	VALUE *ptr = RARRAY_PTR(val);

	char *payload = NULL;
	size_t payload_length = 0;

	size_t i;
	for( i = 0; i < length; ++i)
	{
		VALUE element = ltns_dump( Qnil, ptr[i] );
		if (element == Qnil)
		{
			free( payload );
			return Qnil;
		}

		size_t old_length = payload_length;
		payload_length += RSTRING_LEN(element);
		char * new_payload = (char*)realloc(payload, payload_length);

		if( !new_payload )
		{
			free( payload );
			return Qnil;
		}

		payload = new_payload;
		memcpy( payload + old_length, RSTRING_PTR(element), RSTRING_LEN(element));
	}

	if (payload_length == 0)
		payload = strdup("");

	LTNSTerm *term = NULL;
	LTNSError error = LTNSTermCreate(&term, payload, payload_length, LTNS_LIST);
	if (error)
		return Qnil;
	char* tnetstring = NULL;
	error = LTNSTermGetTNetstring(term, &tnetstring, &length);
	if (error)
		return Qnil;
	VALUE ret = rb_str_new(tnetstring, length);
	LTNSTermDestroy(term);
	free(payload);

	return ret;
}


VALUE ltns_dump_hash(VALUE val)
{
	LTNSPayloadInfo info;
	info.payload = NULL;
	info.length = 0;

	rb_hash_foreach( val, ltns_dump_key_value, (VALUE)&info);
	if (!info.payload && !RHASH_EMPTY_P(val))
		return Qnil;
	if (RHASH_EMPTY_P(val))
		info.payload = strdup("");

	LTNSTerm *term = NULL;
	LTNSError error = LTNSTermCreate(&term, info.payload, info.length, LTNS_DICTIONARY);
	if (error)
		return Qnil;
	char* tnetstring = NULL;
	size_t length = 0;
	error = LTNSTermGetTNetstring(term, &tnetstring, &length);
	if (error)
		return Qnil;
	VALUE ret = rb_str_new(tnetstring, length);
	LTNSTermDestroy(term);
	free(info.payload);

	return ret;
}

static int ltns_dump_key_value( VALUE key, VALUE value, VALUE in)
{
	LTNSPayloadInfo *info = (LTNSPayloadInfo*)in;
	if( !info  )
	{
		return ST_STOP;
	}

	if (TYPE(key) != T_STRING && TYPE(key) != T_SYMBOL)
	{
		free( info->payload );
		info->payload = NULL;
		return ST_STOP;
	}

	VALUE key_payload = ltns_dump( Qnil, key );
	if( key_payload == Qnil )
	{
		free ( info->payload );
		info->payload = NULL;
		return ST_STOP;
	}
	
	VALUE value_payload = ltns_dump( Qnil, value );
	if( value_payload == Qnil )
	{
		free ( info->payload );
		info->payload = NULL;
		return ST_STOP;
	}

	size_t old_length = info->length;
	info->length += RSTRING_LEN(key_payload) + RSTRING_LEN(value_payload);
	char *new_payload = realloc( info->payload, info->length );

	if( !new_payload )
	{
		free( info->payload );
		info->payload = NULL;
		return ST_STOP;
	}

	info->payload = new_payload;
	memcpy( info->payload + old_length, RSTRING_PTR(key_payload), RSTRING_LEN(key_payload));

	memcpy( info->payload + old_length + RSTRING_LEN(key_payload), RSTRING_PTR(value_payload), RSTRING_LEN(value_payload));

	return ST_CONTINUE;
}
