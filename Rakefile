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

file 'ext/Makefile' => 'ext/extconf.rb' do |task|
  Dir::chdir('ext') do
    raise 'Makefile generation failed' unless sh 'ruby extconf.rb'
  end
end

task :build_spec => 'ext/Makefile' do |task|
  Dir::chdir('ext') do
    raise 'build failed' unless sh 'make'
  end
end

task :build_ctests do |task|
  Dir::chdir('test') do
    raise 'build failed' unless sh 'make'
  end
end

task :ctests => :build_ctests do |task|
  raise 'term tests failed' unless sh './test/term_test'
  raise 'data access tests failed' unless sh './test/data_access_test'
end

RSpec::Core::RakeTask.new(:spec) do |t|
  t.rspec_opts = ["--color"]
  t.fail_on_error = false
end

task :clean_tests do |task|
  File.unlink('ext/Makefile')
  Dir['ext/*.o'].each { |file| File.unlink(file) }
  File.unlink('ext/lazy_tnetstring.so') rescue true
  File.unlink('ext/lazy_tnetstring.bundle') rescue true
  File.unlink('test/data_access_test') rescue true
  File.unlink('test/term_test') rescue true
end

task :test => [:ctests, :build_spec, :spec, :clean_tests] do |task|
end

task :run_benchmark => :build_spec do |task|
  Dir::chdir('test/bench') do
    data_files = Dir['data/*.tnet'].join(' ')
    puts '###'
    puts '# Benchmarking full parsing'
    puts '###'
    sh "ruby bench.rb #{data_files}"
    puts '###'
    puts '# Benchmarking lazy parsing'
    puts '###'
    sh "ruby bench.rb lazy #{data_files}"
  end
end

task :benchmark => [:run_benchmark, :clean_tests] do |task|
end

task :build do |task|
  version = File.exist?('VERSION') ? File.read('VERSION').strip : ""
  raise 'gem build failed' unless sh 'gem build lazy_tnetstring.gemspec'
  puts
  puts "Run 'gem install lazy_tnetstring-#{version}.gem' to install."
end

task :default => [:test, :build]

require 'rdoc/task'
RDoc::Task.new do |rdoc|
  version = File.exist?('VERSION') ? File.read('VERSION').strip : ""

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "lazy_tnetstring #{version}"
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('ext/**/*.c')
end
