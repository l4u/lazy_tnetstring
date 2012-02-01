
#include "LTNS.h"

#include "data_access.h"
#include "parse.h"
#include "dump.h"

VALUE cDataAccess;
VALUE cModule;
VALUE eInvalidTNetString;
VALUE eUnsupportedTopLevelDataStructure;
VALUE eInvalidScope;
VALUE eKeyNotFound;

typedef struct _Wrapper
{
	VALUE parent;
	LTNSDataAccess* data_access;
} Wrapper;


static void ltns_da_raise_on_error(LTNSError error);
static VALUE ltns_da_key2str(VALUE key);
static VALUE ltns_da_to_hash_helper(VALUE pair, VALUE hash);


VALUE ltns_da_init(VALUE self)
{
	return self;
}

void ltns_da_mark(void* ptr)
{
	Wrapper *wrapper = (Wrapper*)ptr;
	rb_gc_mark(wrapper->parent);
}

void ltns_da_free(void* ptr)
{
	Wrapper *wrapper = (Wrapper*)ptr;
	LTNSDataAccessDestroy(wrapper->data_access);
	free(wrapper);
}

VALUE ltns_da_new(int argc, VALUE* argv, VALUE class)
{
	VALUE tnetstring = Qnil;
	rb_scan_args(argc, argv, "01", &tnetstring);
	if (tnetstring == Qnil)
	{
		tnetstring = rb_str_new2("0:}"); // Default to empty hash
	}

	if (TYPE(tnetstring) != T_STRING)
	{
		ltns_da_raise_on_error(INVALID_ARGUMENT);
	}

	return ltns_da_new2(class, RSTRING_PTR(tnetstring), RSTRING_LEN(tnetstring));
}

VALUE ltns_da_new2(VALUE class, const char* tnetstring, size_t length)
{
	Wrapper *wrapper = calloc(1, sizeof(Wrapper));
	if (!wrapper)
	{
		ltns_da_raise_on_error(OUT_OF_MEMORY);
	}
	wrapper->parent = Qnil;
	LTNSError error = LTNSDataAccessCreate(&wrapper->data_access, tnetstring, length);
	if (error)
	{
		LTNSDataAccessDestroy(wrapper->data_access);
		free(wrapper);
		ltns_da_raise_on_error(error);
	}

	VALUE tdata = Data_Wrap_Struct(class, ltns_da_mark, ltns_da_free, wrapper);
	rb_obj_call_init(tdata, 0, NULL);
	return tdata;
}

VALUE ltns_da_get(VALUE self, VALUE key)
{
	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);
	key = ltns_da_key2str(key);
	char* key_cstr = StringValueCStr(key);

	LTNSTerm *term = NULL;
	LTNSError error = LTNSDataAccessGet(wrapper->data_access, key_cstr, &term);
	if (error == KEY_NOT_FOUND)
		return Qnil;
	if (error)
	{
		LTNSTermDestroy(term);
		ltns_da_raise_on_error(error);
	}

	VALUE ret = Qnil;

	LTNSType type;
	LTNSTermGetPayloadType(term, &type);
	/* Return a DataAccess object if the value is a dictionary */
	if (type == LTNS_DICTIONARY)
	{
		LTNSDataAccess *child = NULL;

		error = LTNSDataAccessCreateNested(&child, wrapper->data_access, term);
		if (error)
		{
			LTNSTermDestroy(term);
			ltns_da_raise_on_error(error);
		}

		Wrapper *child_wrapper = (Wrapper*)calloc(sizeof(Wrapper), 1);
		if (!child_wrapper)
		{
			LTNSTermDestroy(term);
			ltns_da_raise_on_error(OUT_OF_MEMORY);
		}
		child_wrapper->data_access = child;
		child_wrapper->parent = self;
		ret = Data_Wrap_Struct(cDataAccess, ltns_da_mark, ltns_da_free, child_wrapper);
		rb_obj_call_init(ret, 0, NULL);
	}
	else
	{
		/* If it is not a dictionary parse the tnetstring into a ruby object */
		char* tnetstring;
		size_t length;
		LTNSTermGetTNetstring(term, &tnetstring, &length);
		if (!ltns_parse(tnetstring, tnetstring + length, &ret))
		{
			LTNSTermDestroy(term);
			ltns_da_raise_on_error(INVALID_TNETSTRING);
		}
	}
	LTNSTermDestroy(term);

	return ret;
}

VALUE ltns_da_set(VALUE self, VALUE key, VALUE new_value)
{
	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);
	key = ltns_da_key2str(key);
	char* key_cstr = StringValueCStr(key);

	if (new_value == Qnil)
		return ltns_da_remove(self, key);

	LTNSError error = 0;
	LTNSTerm *term = NULL;
	/* Get the tnetstring if new_value is a DataAccess */
	if (strncmp(rb_class2name(CLASS_OF(new_value)), "LazyTNetstring::DataAccess", 26) == 0)
	{
		Wrapper *wrapper;
		Data_Get_Struct(new_value, Wrapper, wrapper);
		error = LTNSDataAccessAsTerm(wrapper->data_access, &term);
	}
	else /* Other wise just try do dump new_value */
	{
		VALUE new_value_dumped = ltns_dump(cModule, new_value);
		char* new_value_dumped_cstr = StringValueCStr(new_value_dumped);
		error = LTNSTermCreateFromTNestring(&term, new_value_dumped_cstr);
	}
	ltns_da_raise_on_error(error);
	error = LTNSDataAccessSet(wrapper->data_access, key_cstr, term);
	LTNSTermDestroy(term);
	ltns_da_raise_on_error(error);

	return Qnil;
}

VALUE ltns_da_remove(VALUE self, VALUE key)
{
	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);
	key = ltns_da_key2str(key);
	char* key_cstr = StringValueCStr(key);

	LTNSError error = LTNSDataAccessRemove(wrapper->data_access, key_cstr);
	if (error != KEY_NOT_FOUND)
		ltns_da_raise_on_error(error);

	return Qnil;
}

VALUE ltns_da_increment_value(VALUE self, VALUE key, long delta)
{
	VALUE value = ltns_da_get(self, key);
	/* If key doesn't exist set it to zero */
	if (value == Qnil)
	{
		value = LL2NUM(0);
		ltns_da_set(self, key, value);
	}

	if (TYPE(value) == T_FIXNUM || TYPE(value) == T_BIGNUM)
	{
		long long lvalue = NUM2LL(value);
		lvalue += delta;
		return ltns_da_set(self, key, LL2NUM(lvalue));
	}

	return Qnil;
}

VALUE ltns_da_increment_value_ruby(VALUE self, VALUE key)
{
	return ltns_da_increment_value(self, key, 1);
}
VALUE ltns_da_decrement_value_ruby(VALUE self, VALUE key)
{
	return ltns_da_increment_value(self, key, -1);
}

VALUE ltns_da_get_root_tnetstring(VALUE self)
{
	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);

	LTNSDataAccess *root = LTNSDataAccessGetRoot(wrapper->data_access);
	if (!root)
		ltns_da_raise_on_error(INVALID_CHILD);
	LTNSTerm *term = NULL;
	LTNSError error = LTNSDataAccessAsTerm(root, &term);
	ltns_da_raise_on_error(error);
	char* tnetstring;
	size_t length;
	LTNSTermGetTNetstring(term, &tnetstring, &length);
	LTNSTermDestroy(term);
	return rb_str_new(tnetstring, length);
}

VALUE ltns_da_get_tnetstring(VALUE self)
{
	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);

	LTNSTerm *term = NULL;
	LTNSError error = LTNSDataAccessAsTerm(wrapper->data_access, &term);
	ltns_da_raise_on_error(error);
	char* tnetstring;
	size_t length;
	LTNSTermGetTNetstring(term, &tnetstring, &length);
	LTNSTermDestroy(term);
	return rb_str_new(tnetstring, length);
}

VALUE ltns_da_get_offset(VALUE self)
{
	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);

	size_t offset;
	LTNSDataAccessOffset(wrapper->data_access, &offset);
	return LL2NUM(offset);
}

VALUE ltns_da_is_empty(VALUE self)
{
	VALUE scoped_data = ltns_da_get_tnetstring(self);
	char* tnetstring = StringValueCStr(scoped_data);
	if (strncmp(tnetstring, "0:}", 3) == 0)
		return Qtrue;
	return Qfalse;
}

VALUE ltns_da_each(VALUE self)
{
	if (ltns_da_is_empty(self) == Qtrue)
		return Qnil;

	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);

	LTNSTerm *term = NULL;
	LTNSError error = LTNSDataAccessAsTerm(wrapper->data_access, &term);
	ltns_da_raise_on_error(error);
	char* payload;
	size_t payload_length;
	LTNSTermGetPayload(term, &payload, &payload_length, NULL);
	LTNSTermDestroy(term);
	term = NULL;

	size_t length, offset = 0;
	char* tnetstring;

	while (offset < payload_length)
	{
		error = LTNSTermCreateNested(&term, payload + offset, payload + payload_length);
		LTNSTermGetTNetstring(term, &tnetstring, &length);
		LTNSTermDestroy(term);
		ltns_da_raise_on_error(error);

		VALUE key;
		if (!ltns_parse(tnetstring, tnetstring + length, &key))
			ltns_da_raise_on_error(INVALID_TNETSTRING);
		offset += length;

		error = LTNSTermCreateNested(&term, payload + offset, payload + payload_length);
		LTNSTermGetTNetstring(term, &tnetstring, &length);
		LTNSTermDestroy(term);
		ltns_da_raise_on_error(error);

		VALUE value;
		if (!ltns_parse(tnetstring, tnetstring + length, &value))
			ltns_da_raise_on_error(INVALID_TNETSTRING);
		offset += length;

		VALUE pair = rb_ary_new3(2, key, value);
		rb_yield(pair);
	}
	return Qnil;
}

static VALUE ltns_da_to_hash_helper(VALUE pair, VALUE hash)
{
	VALUE key = rb_ary_entry(pair, 0);
	VALUE val = rb_ary_entry(pair, 1);
	rb_hash_aset(hash, key, val);
	return Qnil;
}

VALUE ltns_da_to_hash(VALUE self)
{
	VALUE hash = rb_hash_new();
	rb_block_call(self, rb_intern("each"), 0, NULL, ltns_da_to_hash_helper, hash);
	return hash;
}

static void ltns_da_raise_on_error(LTNSError error)
{
	VALUE rb_Exception;
	switch (error)
	{
	case 0: /* No error */
		break;
	case UNSUPPORTED_TOP_LEVEL_DATA_STRUCTURE:
		rb_raise(eUnsupportedTopLevelDataStructure, "Unsupported top level structure");
	case OUT_OF_MEMORY:
		rb_Exception = rb_const_get(rb_cObject, rb_intern("NoMemoryError"));
		rb_raise(rb_Exception, "Out of memory");
	case INVALID_TNETSTRING:
		rb_raise(eInvalidTNetString, "Invalid TNetstring");
	case INVALID_CHILD:
		rb_raise(eInvalidScope, "Invalid scope");
	case KEY_NOT_FOUND:
		rb_raise(eKeyNotFound, "Key not found");
	default:
		rb_Exception = rb_const_get(rb_cObject, rb_intern("ArgumentError"));
		rb_raise(rb_Exception, "Invalid argument");
	}
}

static VALUE ltns_da_key2str(VALUE key)
{
	VALUE str = key;

	if (TYPE(key) == T_SYMBOL)
	{
		str = rb_sym_to_s(key);
	}
	else if (TYPE(key) != T_STRING)
	{
		str = rb_big2str(rb_hash(key), 10);
	}

	return str;
}

void Init_lazy_tnetstring()
{
	cModule = rb_define_module("LazyTNetstring");
	rb_define_module_function(cModule, "dump", ltns_dump, 1);
	rb_define_module_function(cModule, "parse", ltns_parse_ruby, 1);

	eInvalidTNetString = rb_define_class_under(cModule, "InvalidTNetString", rb_eStandardError);
	eUnsupportedTopLevelDataStructure = rb_define_class_under(cModule, "UnsupportedTopLevelDataStructure", rb_eStandardError);
	eInvalidScope = rb_define_class_under(cModule, "InvalidScope", rb_eStandardError);
	eKeyNotFound = rb_define_class_under(cModule, "KeyNotFound", rb_eStandardError);

	cDataAccess = rb_define_class_under(cModule, "DataAccess", rb_cObject);
	rb_include_module(cDataAccess, rb_mEnumerable);
	rb_define_singleton_method(cDataAccess, "new", ltns_da_new, -1);
	rb_define_method(cDataAccess, "initialize", ltns_da_init, 0);
	rb_define_method(cDataAccess, "[]", ltns_da_get, 1);
	rb_define_method(cDataAccess, "[]=", ltns_da_set, 2);
	rb_define_method(cDataAccess, "remove", ltns_da_remove, 1);
	rb_define_method(cDataAccess, "increment_value", ltns_da_increment_value_ruby, 1);
	rb_define_method(cDataAccess, "decrement_value", ltns_da_decrement_value_ruby, 1);
	rb_define_method(cDataAccess, "data", ltns_da_get_root_tnetstring, 0);
	rb_define_method(cDataAccess, "scoped_data", ltns_da_get_tnetstring, 0);
	rb_define_method(cDataAccess, "offset", ltns_da_get_offset, 0);
	rb_define_method(cDataAccess, "empty?", ltns_da_is_empty, 0);
	rb_define_method(cDataAccess, "each", ltns_da_each, 0);
	rb_define_alias(cDataAccess, "each_pair", "each");
	rb_define_method(cDataAccess, "to_hash", ltns_da_to_hash, 0);
}
