#ifndef __DATA_ACCESS_H__
#define __DATA_ACCESS_H__

#include <ruby.h>

void Init_lazy_tnetstring();

VALUE ltns_da_alloc(VALUE class);
void ltns_da_mark(void* ptr);
void ltns_da_free(void* ptr);
VALUE ltns_da_init(int argc, VALUE* argv, VALUE self);
VALUE ltns_da_get(VALUE self, VALUE key);
VALUE ltns_da_set(VALUE self, VALUE key, VALUE new_value);
VALUE ltns_da_delete(VALUE self, VALUE key);
VALUE ltns_da_get_tnetstring(VALUE self);
VALUE ltns_da_get_offset(VALUE self);
VALUE ltns_da_is_empty(VALUE self);
VALUE ltns_da_each(VALUE self);
VALUE ltns_da_to_hash(VALUE self);
VALUE ltns_da_as_json(int argc, VALUE* argv, VALUE self);
VALUE ltns_da_eql(VALUE self, VALUE other);
VALUE ltns_da_inspect(VALUE self);

#endif
