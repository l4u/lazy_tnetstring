require 'mkmf'
$CFLAGS += ' -Iinclude'
CONFIG['warnflags'] = ' -Werror -Wall' if CONFIG['warnflags']
create_makefile('lazy_tnetstring')
