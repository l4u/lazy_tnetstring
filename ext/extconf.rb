require 'mkmf'
$CFLAGS += ' -I../src'
create_makefile('LazyTNetstring')
makefile = File.read('Makefile')
makefile.gsub!(/^SRCS.*$/) do |match|
  "#{match} ../src/LTNSDataAccess.c ../src/LTNSTerm.c ../src/LTNSCommon.c"
end
makefile.gsub!(/^OBJS.*$/) do |match|
  "#{match} ../src/LTNSDataAccess.o ../src/LTNSTerm.o ../src/LTNSCommon.o"
end
makefile.gsub!(/^warnflags =.*$/) do |match|
   "warnflags = -Werror -Wall"
end
File.open('Makefile', 'w') {|f| f.write(makefile)}
