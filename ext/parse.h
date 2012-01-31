#ifndef __PARSE_H__
#define __PARSE_H__

#include <ruby.h>
#include <stdlib.h>

VALUE ltns_parse_ruby(VALUE module, VALUE string);
int ltns_parse(const char* tnetstring, const char* end, VALUE* out);

int ltns_parse_string(const char* payload, size_t length, VALUE* out);
int ltns_parse_num(const char* payload, size_t length, VALUE* out);
int ltns_parse_float(const char* payload, size_t length, VALUE* out);
int ltns_parse_bool(const char* payload, size_t length, VALUE* out);
int ltns_parse_array(const char* payload, size_t length, VALUE* out);
int ltns_parse_nil(const char* payload, size_t length, VALUE* out);

#endif
