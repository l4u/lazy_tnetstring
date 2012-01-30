require 'mkmf'
$CFLAGS += ' -Iinclude'
CONFIG['warnflags'] = ' -Wall' if CONFIG['warnflags']
create_makefile('lazy_tnetstring')
