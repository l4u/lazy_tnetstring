require 'mkmf'
$LDFLAGS  += '-lLTNS'
create_makefile('LazyTNetstring')
