#!/usr/bin/ruby
#
# Parses a given json file and obfruscates all information
require 'digest/md5'
require 'json'
require 'tnetstring'

def random_integer( length )
	s = (rand(9)+1).to_s # allway start with a !0

	(1..length-1).each do |i|
		s += rand(10).to_s 
	end
	return s;
end

def random_string( length )
	s = ''
	(0..length-1).each do |i|
		char = (rand(26) + 97).chr # 97 == 'a'

		# capitalize if needed
		if( rand(2) == 1 )
			char = char.capitalize
		end

		s += char	
	end
	return s;
end

def crypt (key, char_content = true)

	# randomly add a char to the length
	should_change_size = rand(5) != 0 ? 0 : 1;

	key_pad = ''
	if char_content
		key_pad = random_string( key.to_s.length + should_change_size * (rand(3) -1)  );
	else
		key_pad = random_integer( key.to_s.length + should_change_size * (rand(3) -1)  );
	end

	# pad keys
	return key_pad
end

def obfruscate( element, pad_map )
	# recurse, if type is also a hash
	new_element = nil;
	if element.kind_of? Hash
		new_element = obfruscate_hash( element, pad_map)
	elsif element.kind_of? Array
		new_element = obfruscate_list( element, pad_map)
	elsif element.kind_of? Fixnum
		new_element = crypt(element, false);
	else # string or sth 
		new_element = crypt(element, true);
	end

	return new_element
end


def obfruscate_list( list, pad_map )
	new_content= []
	list.each do |element|
		new_content << obfruscate( element, pad_map );
	end

	return new_content
end

def obfruscate_hash( input_hash, pad_map )
	new_content= {}
	input_hash.each_key do |key|
		pad_map[key] = crypt(key) if pad_map[key] == nil

		old_value = input_hash[key];
		new_content[pad_map[key]] = obfruscate(old_value, pad_map);
	end

	return new_content
end

# check file given
if ARGV.length == 0 
	raise "Please supply a 'json' file as first argument \n"
end

# read json file
for filename in ARGV do
	raise "File not found!" if !filename

	json_content = File.open(filename,'r').read
	ruby_content = JSON.parse(json_content)

	new_content = obfruscate( ruby_content, {} )
	new_json    = JSON.dump(new_content)
	new_tnet    = TNetstring.dump(new_content)

	# write file in current folder
	File.open( File.basename( filename ), 'w'). write(new_json);
	File.open( File.basename( filename, '.json' ) + '.tnet', "w" ).write(new_tnet);

	# debug
	puts 'done obfruscating "./' + File.basename(filename, '.json') + ".*'"
end
