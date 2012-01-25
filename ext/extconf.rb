require 'mkmf'
$CFLAGS += ' -I../src -Werror -Wall'
create_makefile('LazyTNetstring')
makefile = File.read('Makefile')
makefile.gsub!(/^SRCS.*$/) do |match|
  "#{match} ../src/LTNSDataAccess.c ../src/LTNSTerm.c ../src/LTNSCommon.c"
end
makefile.gsub!(/^OBJS.*$/) do |match|
  "#{match} ../src/LTNSDataAccess.o ../src/LTNSTerm.o ../src/LTNSCommon.o"
end
File.open('Makefile', 'w') {|f| f.write(makefile)}
