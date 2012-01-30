#!/usr/bin/ruby
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

def randomize_length (key, char_content = true)

	# randomly add a char to the length
	should_change_size = rand(2) != 0 ? 0 : 1;

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
		new_element = randomize_length(element, false);
	else # string or sth 
		new_element = randomize_length(element, true);
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
		pad_map[key] = randomize_length(key) if pad_map[key] == nil

		old_value = input_hash[key];
		new_content[pad_map[key]] = obfruscate(old_value, pad_map);
	end

	return new_content
end

