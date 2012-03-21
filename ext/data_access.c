
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
	VALUE key_mappings;
	LTNSDataAccess* data_access;
} Wrapper;


static void ltns_da_raise_on_error(LTNSError error);
static VALUE ltns_da_key2str(VALUE key);
static VALUE ltns_da_prepare_key(VALUE self, VALUE key);
static VALUE ltns_da_to_hash_helper(VALUE pair, VALUE hash);


VALUE ltns_da_alloc(VALUE class)
{
	Wrapper *wrapper = calloc(1, sizeof(Wrapper));
	if (!wrapper)
		ltns_da_raise_on_error(OUT_OF_MEMORY);

	wrapper->parent = Qnil;
	wrapper->data_access = NULL;

	VALUE obj = Data_Wrap_Struct(class, ltns_da_mark, ltns_da_free, wrapper);
	return obj;
}

void ltns_da_mark(void* ptr)
{
	Wrapper *wrapper = (Wrapper*)ptr;
	rb_gc_mark(wrapper->parent);
	rb_gc_mark(wrapper->key_mappings);
}

void ltns_da_free(void* ptr)
{
	Wrapper *wrapper = (Wrapper*)ptr;
	if (wrapper->data_access)
		LTNSDataAccessDestroy(wrapper->data_access);
	free(wrapper);
}

VALUE ltns_da_init(int argc, VALUE* argv, VALUE self)
{
	VALUE tnetstring = Qnil;
	VALUE options = Qnil;
	rb_scan_args(argc, argv, "02", &tnetstring, &options);
	if (tnetstring == Qnil)
	{
		tnetstring = rb_str_new2("0:}"); // Default to empty hash
	}

	if (TYPE(tnetstring) != T_STRING)
	{

		ltns_da_raise_on_error(INVALID_ARGUMENT);
	}

	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);

	LTNSError error = LTNSDataAccessCreate(&wrapper->data_access, RSTRING_PTR(tnetstring), RSTRING_LEN(tnetstring));
	ltns_da_raise_on_error(error);

	wrapper->key_mappings = Qnil;
	if (TYPE(options) == T_HASH)
	{
		VALUE sym_mappings = ID2SYM(rb_intern("mappings"));
		wrapper->key_mappings = rb_hash_aref(options, sym_mappings);
	}

	if (TYPE(wrapper->key_mappings) != T_HASH)
		wrapper->key_mappings = rb_hash_new();

	return self;
}

VALUE ltns_da_get(VALUE self, VALUE key)
{
	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);
	key = ltns_da_prepare_key(self, key);
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

		ret = ltns_da_alloc(cDataAccess);
		Wrapper* child_wrapper;
		Data_Get_Struct(ret, Wrapper, child_wrapper);
		child_wrapper->data_access = child;
		child_wrapper->parent = self;
		child_wrapper->key_mappings = wrapper->key_mappings;
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
	key = ltns_da_prepare_key(self, key);
	char* key_cstr = StringValueCStr(key);

	if (new_value == Qnil)
		return ltns_da_delete(self, key);

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

VALUE ltns_da_delete(VALUE self, VALUE key)
{
	VALUE ret = ltns_da_get(self, key);

	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);
	key = ltns_da_prepare_key(self, key);
	char* key_cstr = StringValueCStr(key);

	LTNSError error = LTNSDataAccessRemove(wrapper->data_access, key_cstr);
	if (error != KEY_NOT_FOUND)
		ltns_da_raise_on_error(error);

	return ret;
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
	if (!rb_block_given_p())
		rb_raise(rb_eArgError, "No block given!");

	if (ltns_da_is_empty(self) == Qtrue)
		return Qnil;

	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);

	VALUE key_mappings_inverse = rb_funcall(wrapper->key_mappings, rb_intern("invert"), 0);

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

		VALUE mapped_key = rb_hash_aref(key_mappings_inverse, key);
		if (mapped_key != Qnil)
			key = mapped_key;

		error = LTNSTermCreateNested(&term, payload + offset, payload + payload_length);
		LTNSTermGetTNetstring(term, &tnetstring, &length);
		LTNSTermDestroy(term);
		ltns_da_raise_on_error(error);

		/* FIXME: Calling get for value turns each into O(n^2) */
		/* NOTE: Can't call parse for value since we want a nested DA,
		 * parse only creates new roots */
		VALUE value = ltns_da_get(self, key);

		offset += length;

		VALUE pair = rb_ary_new3(2, key, value);
		rb_yield(pair);

		/* FIXME: If the block we yield to modifies the TNetstring this may break! */

		/* Update data location and length */
		error = LTNSDataAccessAsTerm(wrapper->data_access, &term);
		ltns_da_raise_on_error(error);
		LTNSTermGetPayload(term, &payload, &payload_length, NULL);
		LTNSTermDestroy(term);
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

VALUE ltns_da_as_json(int argc, VALUE* argv, VALUE self)
{
	/* FIXME: Support options argument */
	return ltns_da_to_hash(self);
}

VALUE ltns_da_initialize_copy(VALUE copy, VALUE orig)
{
	if (copy == orig)
		return copy;

	Wrapper *copy_wrapper, *orig_wrapper;
	Data_Get_Struct(copy, Wrapper, copy_wrapper);
	Data_Get_Struct(orig, Wrapper, orig_wrapper);

	if (TYPE(orig) != T_DATA || RDATA(orig)->dfree != (RUBY_DATA_FUNC)ltns_da_free)
		rb_raise(rb_eTypeError, "Wrong argument type");

	if (orig_wrapper->parent != Qnil || copy_wrapper->parent != Qnil)
		rb_raise(rb_eTypeError, "Can't copy nested DataAccess");

	if (copy_wrapper->data_access)
		ltns_da_raise_on_error(INVALID_ARGUMENT);

	/* Get tnetstring from original */
	LTNSTerm *term = NULL;
	LTNSError error = LTNSDataAccessAsTerm(orig_wrapper->data_access, &term);
	ltns_da_raise_on_error(error);
	char* tnetstring;
	size_t length;
	LTNSTermGetTNetstring(term, &tnetstring, &length);
	LTNSTermDestroy(term);

	/* Create new root copy from original */
	copy_wrapper->parent = Qnil;
	copy_wrapper->key_mappings = orig_wrapper->key_mappings;
	error = LTNSDataAccessCreate(&copy_wrapper->data_access, tnetstring, length);
	ltns_da_raise_on_error(error);

	return copy;
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

static VALUE ltns_da_prepare_key(VALUE self, VALUE key)
{
	Wrapper *wrapper;
	Data_Get_Struct(self, Wrapper, wrapper);

	VALUE mapped_key = rb_hash_aref(wrapper->key_mappings, key);

	if (mapped_key != Qnil)
		key = mapped_key;

	return ltns_da_key2str(key);
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
	rb_define_alloc_func(cDataAccess, ltns_da_alloc);
	rb_include_module(cDataAccess, rb_mEnumerable);

	rb_define_method(cDataAccess, "initialize", ltns_da_init, -1);
	rb_define_method(cDataAccess, "[]", ltns_da_get, 1);
	rb_define_method(cDataAccess, "[]=", ltns_da_set, 2);
	rb_define_method(cDataAccess, "delete", ltns_da_delete, 1);
	rb_define_method(cDataAccess, "increment_value", ltns_da_increment_value_ruby, 1);
	rb_define_method(cDataAccess, "decrement_value", ltns_da_decrement_value_ruby, 1);
	rb_define_method(cDataAccess, "data", ltns_da_get_root_tnetstring, 0);
	rb_define_method(cDataAccess, "scoped_data", ltns_da_get_tnetstring, 0);
	rb_define_method(cDataAccess, "offset", ltns_da_get_offset, 0);
	rb_define_method(cDataAccess, "empty?", ltns_da_is_empty, 0);
	rb_define_method(cDataAccess, "each", ltns_da_each, 0);
	rb_define_alias(cDataAccess, "each_pair", "each");
	rb_define_method(cDataAccess, "to_hash", ltns_da_to_hash, 0);
	rb_define_method(cDataAccess, "as_json", ltns_da_as_json, -1);
	rb_define_method(cDataAccess, "initialize_copy", ltns_da_initialize_copy, 1);
}
