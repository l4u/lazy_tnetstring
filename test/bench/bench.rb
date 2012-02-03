#!/usr/bin/ruby
# 
# benchmark obfruscated data

require 'benchmark'
require 'lazy_tnetstring'
require 'tnetstring'
require './helper.rb'

def change_array_content(array, keys)
	key = rand(array.length)
	value = array[key]
	
	array[key] = change_content(value, keys)
	return array
end

def find_random_key(data, keys)
	key_id = rand(keys.length)
	key = keys[key_id]
	
	max_loops = keys.length/2
	while( data[ keys[key_id] ].nil? == true && max_loops > 0 )
		key_id = rand(keys.length)
		key = keys[key_id]

		max_loops -= 1
	end
	
	key = keys[key_id]
	return key
end

def change_data_access_content(data_access, keys)
	key = find_random_key(data_access, keys)
	value = data_access[key]

	data_access[key] = change_content(data_access[key], keys)
	
	return data_access
end

def change_content(data, keys)
	if( !data )
		return
	end

	# change key accordingly
	new_value = nil
	if( data.kind_of?(LazyTNetstring::DataAccess) || data.kind_of?(Hash))
		new_value = change_data_access_content( data, keys)
	elsif( data.kind_of? Array )
		new_value = change_array_content( data, keys )
	elsif( data.to_i != 0 )
		new_value = data.to_i
		new_value += rand(100) - 50
	elsif( data.kind_of? String )
		new_value = randomize_length( data )
	else
		raise "Type " + data.class + " cannot be parsed!"
	end
	data = new_value

	return new_value
end

def parse(use_lazy, content)
	if( use_lazy )
		return LazyTNetstring::DataAccess.new( content ) 
	else
		return TNetstring::parse( content )[0]
	end
end

def dump(use_lazy, data )
	if( use_lazy )
		return data.data;
	else
		return TNetstring.dump( data )
	end
end

def main
	raise "No file(s) given!" if ARGV.length < 1

	# check if first file is an integer
	loops = 1000
	if( ARGV[0].to_i > 0 )
		loops = ARGV[0].to_i
		puts( 'loop count of ' + loops.to_s + ' found' + "\n\n")
		ARGV.shift
	end

	use_lazy = false
	if( ARGV[0] == "lazy" )
		use_lazy = true
		ARGV.shift
	end

	# loop through all files
	Benchmark.bm do |bench|
		for file in ARGV do bench.report(file+":") {
			# read list
			keys = nil
			keyfile = File.open(File.dirname(file) + '/' + File.basename(file, '.tnet') + ".keys", 'r')
			if !keyfile
				raise "Could not open file '" + file + "'."
			end

			keys = keyfile.read.split("\n")

			if !keys || !file
				break;
			end

			file_content = File.open(file, 'r').read
			if( !file_content )
				raise "Could not open/parse file '" + file +"'"
			end
	 
			for i in (0..(loops-1)) do
				data_before = parse(use_lazy, file_content)
				ltns_before = dump(use_lazy, data_before)
		
				data_after = change_content( data_before, keys)
				ltns_after  = dump(use_lazy, data_after)
		
				if( loops < 10 )
					puts( "ltns was: " +  ltns_before)
					puts( "      is: " +  ltns_after + "\n\n")
				end
			end
		} end
	end
end

main
