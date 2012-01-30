#!/usr/bin/ruby
#
# Parses a given json file and obfruscates all information

require './helper.rb'

# check file given
if ARGV.length == 0 
	raise "Please supply a 'json' file as first argument \n"
end

# read json file
for filename in ARGV do
	raise "File not found!" if !filename

	json_content = File.open(filename,'r').read
	ruby_content = JSON.parse(json_content)

	key_map = {}
	new_content = obfruscate( ruby_content, key_map)
	new_json    = JSON.dump(new_content)
	new_tnet    = TNetstring.dump(new_content)

	# write file in current folder
#	File.open( File.basename( filename ), 'w'). write(new_json)
	File.open( File.basename( filename, '.json' ) + '.tnet', "w" ).write(new_tnet)

	# write keys to file
	keys_file = File.open( File.basename( filename, '.json') + ".keys", "w")
	for key in key_map.keys do
		keys_file.write( key_map[key] )
		keys_file.write( "\n" )
	end
	keys_file.close
		
	# debug
	puts 'done obfruscating "./' + File.basename(filename, '.json') + ".*'"
end
