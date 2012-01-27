#!/usr/bin/ruby
# 
# benchmark obfruscated data

require '../../ext/LazyTNetstring.so'

loops = 1000;

# loop through all files
for file in ARGV do

	# read list
	keys = File.open(File.open(file + ".keys", 'r').read if file

	if !keys || !file
		break;
	end

	for i in (0..loops) do  
		file_content = File.open(file, 'r').read if file
		if( !file_content )
			raise "Could not open/parse file '" + file +"'"
		end

		data = LazyTNetstring::DataAccess.new ( file_content )
		if( !data )
			raise "'"+file+"' is not a valid tnetstring!"
		end

		keys = data.keys
		key_id = random(keys.length)

		value = data[keys[key_id]];
		new_value = value;
		
		# change key accordingly
		if( key_id.kind_of? Fixnum )
			puts "changing integer value"
			new_value += rand(100) - 50
		end
		
		puts "  was data[" + key_id.to_s + "] = " + value.to_s
		puts "  is now " + value.to_s
	end
end

