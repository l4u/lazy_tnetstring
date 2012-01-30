$:.unshift(File.join(File.dirname(__FILE__), '..', 'ext'))
require 'rspec'
require 'lazy_tnetstring'

RSpec.configure do |config|
  config.mock_with :rspec
  config.color_enabled = true
end
