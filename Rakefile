require 'rake'
require 'rubygems'

require 'rspec/core/rake_task'
RSpec::Core::RakeTask.new(:spec) do |t|
  t.rspec_opts = ["--color"]
  t.fail_on_error = false
end

RSpec::Core::RakeTask.new(:test) do |t|
  system('cd test && make 2>&1 > /dev/null') || raise('Failed to build C tests')
  puts 'Running data_access_test'
  system('./test/data_access_test') || raise('C tests failed')
  puts 'Running term_test'
  system('./test/term_test') || raise('C tests failed')
end

task :default => :spec

require 'jeweler'
Jeweler::Tasks.new do |gem|
  gem.name = "LazyTNetstring"
  gem.homepage = "http://github.com/chrisavl/CLazyTNetstring"
  gem.license = "MIT"
  gem.summary = 'Lazy TNetstring C implementation.'
  gem.description = 'A fast C-based lazy TNetstring data accessor.'
  gem.email = "christian.lundgren@wooga.net"
  gem.authors = ["Christian Lundgren"]
  gem.test_files = `git ls-files -- {test,spec,features}/*`.split("\n")
  gem.files = `git ls-files -- {ext,COPYING,README,VERSION}`.split("\n")
  gem.version = File.exist?('VERSION') ? File.read('VERSION') : ""

  gem.add_development_dependency 'tnetstring'
  gem.add_development_dependency 'rake'
  gem.add_development_dependency 'rspec', '~> 2'
  gem.add_development_dependency 'autotest'
  gem.add_development_dependency 'activesupport'
  gem.add_development_dependency 'i18n'
end
Jeweler::RubygemsDotOrgTasks.new

require 'rdoc/task'
RDoc::Task.new do |rdoc|
  version = File.exist?('VERSION') ? File.read('VERSION') : ""

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "LazyTNetstring #{version}"
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('ext/**/*.c')
end
