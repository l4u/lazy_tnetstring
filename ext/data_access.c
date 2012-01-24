#include <ruby.h>

#include "LTNS.h"

extern VALUE ltns_dump(VALUE val);
extern VALUE ltns_parse_ruby(VALUE string);

typedef struct _Wrapper
{
	VALUE parent;
	LTNSDataAccess* data_access;
} Wrapper;

VALUE cDataAccess;
VALUE cModule;

VALUE ltns_da_init(VALUE self);
void ltns_da_mark(void* ptr);
void ltns_da_free(void* ptr);
VALUE ltns_da_new(VALUE class, VALUE dict);

void ltns_da_raise_on_error(LTNSError error);

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
	LTNSError error = LTNSDataAccessDestroy(wrapper->data_access);
	free(wrapper);
}

VALUE ltns_da_new(VALUE class, VALUE dict)
{
	VALUE tnetstring = ltns_dump(dict);
	
	Wrapper *wrapper = calloc(1, sizeof(Wrapper));
	if (!wrapper)
	{
		VALUE rb_eNoMemoryError = rb_const_get(rb_cObject, rb_intern("NoMemoryError"));
		rb_raise(rb_eNoMemoryError, "Out of memory");
	}
	wrapper->parent = Qnil;
	LTNSError error = LTNSDataAccessCreate(&wrapper->data_access, RSTRING_PTR(tnetstring), RSTRING_LEN(tnetstring));
	if (error)
	{
		if (wrapper->data_access)
		{
			LTNSDataAccessDestroy(wrapper->data_access);
			free(wrapper);
		}
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
	char* key_cstr = StringValueCStr(key);

	LTNSTerm *term = NULL;
	LTNSError error = LTNSDataAccessGet(wrapper->data_access, key_cstr, &term);

	free(key_cstr);
	if (error)
	{
		LTNSTermDestroy(term);
		ltns_da_raise_on_error(error);
	}

	VALUE ret;
	char* tnetstring;
	error = LTNSTermGetTNetstring(term, &tnetstring, NULL);
	if (!ltns_parse(tnetstring, &ret))
	{
		LTNSTermDestroy(term);
		ltns_da_raise_on_error(INVALID_TNETSTRING);
	}
	LTNSTermDestroy(term);

	return ret;
}

void ltns_da_raise_on_error(LTNSError error)
{
	VALUE rb_Exception;
	switch (error)
	{
	case 0: /* No error */
		break;
	case OUT_OF_MEMORY:
		rb_Exception = rb_const_get(rb_cObject, rb_intern("NoMemoryError"));
		rb_raise(rb_Exception, "Out of memory");
	case INVALID_TNETSTRING:
		rb_Exception = rb_const_get(rb_cObject, rb_intern("ArgumentError"));
		rb_raise(rb_Exception, "Invalid TNetstring");
	case UNSUPPORTED_TOP_LEVEL_DATA_STRUCTURE:
	default:
		rb_Exception = rb_const_get(rb_cObject, rb_intern("ArgumentError"));
		rb_raise(rb_Exception, "Invalid argument");
	}
}

void Init_LazyTNetstring()
{
	cModule = rb_define_module("LazyTNetstring");
	rb_define_module_function(cModule, "dump", ltns_dump, 1);
	rb_define_module_function(cModule, "parse", ltns_parse_ruby, 1);

	cDataAccess = rb_define_class_under(cModule, "DataAccess", rb_cObject);
	rb_define_singleton_method(cDataAccess, "new", ltns_da_new, 0);
	rb_define_method(cDataAccess, "initialize", ltns_da_init, 0);
}
