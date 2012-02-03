Gem::Specification.new do |gem|
  gem.name = "lazy_tnetstring"
  gem.homepage = "http://github.com/chrisavl/CLazyTNetstring"
  gem.license = "MIT"
  gem.summary = 'Lazy TNetstring C implementation.'
  gem.description = 'A fast C-based lazy TNetstring data accessor.'
  gem.email = ["christian.lundgren@wooga.net", "mario.bodemann@wooga.net"]
  gem.authors = ["Christian Lundgren", "Mario Bodemann"]
  gem.test_files = `git ls-files -- {test,spec,features}/*`.split("\n")
  gem.files = `git ls-files -- {ext,COPYING,README,VERSION}`.split("\n")
  gem.extensions = ["ext/extconf.rb"]
  gem.version = File.exist?('VERSION') ? File.read('VERSION').strip : ""
  gem.licenses = ["MIT"]
  gem.require_paths = ["ext"]

  gem.add_development_dependency 'tnetstring'
  gem.add_development_dependency 'rake'
  gem.add_development_dependency 'rspec', '~> 2'
  gem.add_development_dependency 'autotest'
  gem.add_development_dependency 'activesupport'
  gem.add_development_dependency 'i18n'
end

