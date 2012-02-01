#ifndef __DATA_ACCESS_H__
#define __DATA_ACCESS_H__

#include <ruby.h>

void Init_lazy_tnetstring();

VALUE ltns_da_init(VALUE self);
void ltns_da_mark(void* ptr);
void ltns_da_free(void* ptr);
VALUE ltns_da_new(int argc, VALUE* argv, VALUE class);
VALUE ltns_da_new2(VALUE class, const char* tnetstring, size_t length);
VALUE ltns_da_get(VALUE self, VALUE key);
VALUE ltns_da_set(VALUE self, VALUE key, VALUE new_value);
VALUE ltns_da_remove(VALUE self, VALUE key);
VALUE ltns_da_get_tnetstring(VALUE self);
VALUE ltns_da_get_offset(VALUE self);
VALUE ltns_da_is_empty(VALUE self);
VALUE ltns_da_each(VALUE self);
VALUE ltns_da_to_hash(VALUE self);

#endif
