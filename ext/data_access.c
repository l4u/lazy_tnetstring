#include <ruby.h>

#include "LTNS.h"

extern VALUE ltns_dump(VALUE val);

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
	const char* tnetstring = "0:}";
	// FIXME: dump ruby dict to tnetstring
	
	Wrapper *wrapper = malloc(sizeof(Wrapper));
	if (!wrapper)
	{
		// FIXME: raise exception
	}
	wrapper->parent = Qnil;
	LTNSError error = LTNSDataAccessCreate(&wrapper->data_access, tnetstring, strlen(tnetstring));
	if (error)
	{
		// FIXME: raise exception
	}
	VALUE tdata = Data_Wrap_Struct(class, ltns_da_mark, ltns_da_free, wrapper);
	rb_obj_call_init(tdata, 0, NULL);
	return tdata;
}

void Init_LazyTNetstring()
{
	cModule = rb_define_module("LazyTNetstring");
	rb_define_module_function(cModule, "dump", ltns_dump, 1);

	cDataAccess = rb_define_class_under(cModule, "DataAccess", rb_cObject);
	rb_define_singleton_method(cDataAccess, "new", ltns_da_new, 0);
	rb_define_method(cDataAccess, "initialize", ltns_da_init, 0);
}
