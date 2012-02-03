# Lazy TNetstring

Data Accessor to lazy parse and update tnetstrings, implemented in C.

## Usage

Say you have a tnetstring (see <http://tnetstrings.org/>) assigned to `data`

    data = "92:4:key1,6:value1,5:inner,48:4:key1,13:inner value 1,4:key2,13:inner value 2,}4:key2,6:value2,}"

that resembles the following Ruby Hash

    {
      'key1' => 'value1',
      'inner' => {
        'key1' => 'inner value 1',
        'key2' => 'inner value 2'
      },
      'key2' => 'value2'
    }

you can interact with it like this

    >> require 'lazy_tnetstring'
    => true

    >> da = LazyTNetstring::DataAccess.new(data)
    => #<LazyTNetstring::DataAccess ...>

    # hash-like access
    >> da['inner']['key2']
    => "inner value 2"

    # updating values
    >> da['inner']['key2'] = 'new value'
    => "new value"

    # your data is not updated in place
    >> data
    => "92:4:key1,6:value1,5:inner,48:4:key1,13:inner value 1,4:key2,13:inner value 2,}4:key2,6:value2,}" 
    >> da.data
    => "87:4:key1,6:value1,5:inner,43:4:key1,13:inner value 1,4:key2,9:new value,}4:key2,6:value2,}"

    # accessing unknown keys yields nil
    >> da['nonexisting']
    => nil

## Installation

    rake build
    gem install pkg/LazyTNetstring-*.gem

## Performance

If you want to compare the performance of lazy parsing/updating to parsing a
full tnetstring some simple benchmarks are included. To compare the time it
takes to parse and update 1000 random keys with the tnetstring library versus
this lazy\_tnetstring library simply run:

    rake benchmark

## Copyright

Copyright (c) 2011 wooga GmbH <http://www.wooga.com>. See MIT-LICENSE for details.
