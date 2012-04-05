#ifndef __DUMP_H__
#define __DUMP_H__

#include <ruby.h>

VALUE ltns_dump(VALUE module, VALUE val);

VALUE ltns_dump_by_to_s(VALUE val, LTNSType type);

VALUE ltns_dump_bool(VALUE val);
VALUE ltns_dump_string(VALUE val);
VALUE ltns_dump_array(VALUE val);
VALUE ltns_dump_hash(VALUE val);
VALUE ltns_dump_nil(VALUE val);

#endif
