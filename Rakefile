require 'rubygems'
require 'bundler'
begin
  Bundler.setup(:default, :development)
rescue Bundler::BundlerError => e
  $stderr.puts e.message
  $stderr.puts "Run `bundle install` to install missing gems"
  exit e.status_code
end
require 'rake'

require 'rspec/core/rake_task'
RSpec::Core::RakeTask.new(:spec) do |t|
  # Hack to build binary needed for tests
  sh 'cd ext && ruby extconf.rb && make clean && make 2>&1 > /dev/null'

  t.rspec_opts = ["--color"]
  t.fail_on_error = false
end

RSpec::Core::RakeTask.new(:test) do |t|
  sh('cd test && make 2>&1 > /dev/null') || raise('Failed to build C tests')
  puts 'Running data_access_test'
  sh('./test/data_access_test') || raise('C tests failed')
  puts 'Running term_test'
  sh('./test/term_test') || raise('C tests failed')
end

task :default => :spec

require 'rdoc/task'
RDoc::Task.new do |rdoc|
  version = File.exist?('VERSION') ? File.read('VERSION') : ""

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "lazy_tnetstring #{version}"
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('ext/**/*.c')
end
