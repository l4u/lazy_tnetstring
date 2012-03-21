require 'spec_helper'
require 'tnetstring'

module LazyTNetstring
  describe DataAccess do

    describe '#new' do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context 'for no argument to constructor' do
        let(:data_access) { LazyTNetstring::DataAccess.new }

        it 'should create an empty hash' do
          subject.data.should == '0:}'
        end
      end

      context 'when passing nil to constructor' do
        let(:data) { nil }

        it 'should create an empty hash' do
          subject.data.should == '0:}'
        end
      end

      context 'for non-tnetstring compliant data' do
        let(:data) { '12345}' }

        it 'rejects initialization' do
          expect { subject }.to raise_error(LazyTNetstring::InvalidTNetString)
        end
      end

      context 'for anything but a hash at the top level' do
        let(:data) { TNetstring.dump(1) }

        it 'rejects initialization' do
          expect { subject }.to raise_error(LazyTNetstring::UnsupportedTopLevelDataStructure)
        end
      end

      context 'for an empty hash' do
        let(:data) { TNetstring.dump({}) }

        it { should be_an LazyTNetstring::DataAccess }
        its(:data)   { should == data }
        its(:offset) { should == 0 }
      end

      context 'for a hash' do
        let(:data) { TNetstring.dump({'key' => 'value', 'another' => 'value'}) }

        it { should be_an LazyTNetstring::DataAccess }
        its(:data)   { should == data }
        its(:offset) { should == 0 }
      end

      context 'with an options hash' do
        let(:data) { TNetstring.dump({short_key => 'value'}) }
        let(:long_key) { 'x' * 100 }
        let(:short_key) { 'x' }
        let(:mappings) { {long_key => short_key} }
        let(:data_access) { LazyTNetstring::DataAccess.new(data, :mappings => mappings) }

        its(:data) { should == data }
        its(:to_hash) { should == {long_key => 'value'} }
      end
    end

    describe '#[]' do
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }
      subject   { data_access[key]}
      let(:key) { 'foo' }

      context 'for empty hash' do
        let(:data) { TNetstring.dump({}) }
        it { should be_nil }
      end

      context 'for unknown keys' do
        let(:data) { TNetstring.dump({'baz' => 'bar'}) }
        it { should be_nil }
      end

      context 'for known keys' do
        let(:data) { TNetstring.dump({'foo' => 'bar'}) }
        it { should == 'bar' }
      end

      context 'for nested hash' do
        let(:data) { TNetstring.dump({'outer' => { 'inner' => 'value'} }) }
        let(:key)  { 'outer' }

        it { should be_an LazyTNetstring::DataAccess }
        its(:scoped_data) { should == TNetstring.dump({ 'inner' => 'value'}) }

        it 'should provide access to the inner hash' do
          subject['inner'].should == 'value'
        end
      end

      context 'for nested hash with non-existing key' do
        let(:data)              { TNetstring.dump({'outer' => { 'inner' => 'value'}, 'user' => {} }) }
        let(:key)               { 'outer' }
        let(:non_existing_key)  { 'non_existing_key' }

        it 'should not find a value and terminate search' do
          subject[non_existing_key].should be_nil
        end
      end

      context 'with key mappings' do
        let(:data)  { TNetstring.dump({ 'key' => 'value' }) }
        let(:mappings) { {'long-key' => 'key'} }
        let(:data_access) { LazyTNetstring::DataAccess.new(data, :mappings => mappings) }

        context "when accessing through the key mappings" do
          let(:key) { 'long-key' }

          it { should == 'value' }
        end

        context "when accessing without the key mappings" do
          let(:key) { 'key' }

          it { should == 'value' }
        end
      end
    end

    describe '#[]=(key, new_value)' do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }
      let(:data)        { TNetstring.dump({key => old_value}) }
      let(:key)         { 'foo' }
      let(:old_value)   { 'bar' }
      let(:new_value)   { 'baz' }

      context 'for single value updates' do
        before( :each ) do
          subject[key] = new_value
        end

        context 'whithout changing the length' do
          its(:data) { should == data.sub('bar', 'baz') }
        end

        context "when changing the length of a top level key's value" do
          let(:data)      { TNetstring.dump({key => old_value}) }
          let(:new_value) { 'x' * 100 }
          let(:new_data)  { TNetstring.dump({key => new_value}) }

          it 'should update the value in its data and adjust lengths accordingly' do
            subject.data.should == new_data
            subject[key].should == new_value
          end
        end
      end

      context "when changing the length of a nested key's value" do
        let(:data)      { TNetstring.dump('outer' => {key => old_value}) }
        let(:new_value) { 'x' * 100 }
        let(:new_data)  { TNetstring.dump('outer' => {key => new_value}) }

        it 'should update the value in its data and adjust lengths accordingly' do
          subject['outer'][key] = new_value
          subject.data.should == new_data
          subject['outer'][key].should == new_value
        end
      end

      context "when changing a nested key's value without changing the length" do
        let(:data)      { TNetstring.dump('outer' => {key => old_value}) }
        let(:new_value) { 'x' * old_value.length }
        let(:new_data)  { TNetstring.dump('outer' => {key => new_value}) }

        it 'should update the value in its data and adjust lengths accordingly' do
          subject['outer'][key] = new_value
          subject.data.should == new_data
          subject['outer'][key].should == new_value
        end
      end

      context "when changing multiple values on different levels" do
        let(:data)      { TNetstring.dump(key => old_value, 'outer' => {key => old_value}) }
        let(:new_value) { 'x' * 100 }
        let(:new_data)  { TNetstring.dump(key => new_value, 'outer' => {key => new_value}) }

        it 'should update the values in its data and adjust lengths accordingly' do
          subject['outer'][key] = new_value
          subject[key] = new_value
          subject.data.should == new_data
          subject[key].should == new_value
          subject['outer'][key].should == new_value
        end
      end

      context "when changing multiple values on different levels while re-using scoped data_accesses" do
        let(:data)      { TNetstring.dump({
                            'key1' => old_value,
                            'outer' => {
                              'key1' => old_value,
                              'key2' => old_value
                            },
                            'key2' => old_value
                          })}
        let(:new_value) { 'x' * 100 }
        let(:new_data)  { TNetstring.dump({
                            'key1' => new_value,
                            'outer' => {
                              'key1' => new_value,
                              'key2' => new_value
                            },
                            'key2' => new_value
                          })}

        it 'should update the values in its data and adjust lengths accordingly' do
          scoped_data_access = subject['outer']
          scoped_data_access['key1'] = new_value
          scoped_data_access['key2'] = new_value
          subject['key1'] = new_value
          subject['key2'] = new_value
          subject.data.should == new_data
          subject['key1'].should == new_value
          subject['key2'].should == new_value
          subject['outer']['key1'].should == new_value
          subject['outer']['key2'].should == new_value
        end
      end

      context "when changing multiple interleaved values on different levels while re-using scoped data_accesses" do
        let(:data)      { TNetstring.dump({
                            'key1' => old_value,
                            'outer' => {
                              'key1' => old_value,
                              'key2' => old_value
                            },
                            'key2' => old_value
                          })}
        let(:new_value) { 'x' * 100 }
        let(:new_data)  { TNetstring.dump({
                            'key1' => new_value,
                            'outer' => {
                              'key1' => new_value,
                              'key2' => new_value
                            },
                            'key2' => new_value
                          })}

        it 'should update the values in its data and adjust lengths accordingly' do
          scoped_data_access = subject['outer']
          subject['key1'] = new_value
          scoped_data_access['key1'] = new_value
          subject['key2'] = new_value
          scoped_data_access['key2'] = new_value
          subject.data.should == new_data
          subject['key1'].should == new_value
          subject['key2'].should == new_value
          subject['outer']['key1'].should == new_value
          subject['outer']['key2'].should == new_value
        end
      end

      context "when updating inner hashes so that references to old keys get invalid" do
        let(:data)      { TNetstring.dump({
                            'level1' => {
                              'level2' => {
                                'level3' => {'key' => old_value }
                              }
                            }
                          })}
        let(:new_value) { 'x' * 100 }
        let(:new_data)  { TNetstring.dump({
                            'level1' => {
                              'level2' => {
                                'newlevel3' => {'key' => new_value }
                              }
                            }
                          })}

        it 'should raise InvalidScope when trying to access removed scopes' do
          level1 = subject['level1']
          level2 = level1['level2']
          level3 = level2['level3']
          level1['level2'] = { 'newlevel3' => { 'key' => new_value } }
          subject.data.should == new_data
          expect { level3['key'] }.to raise_error(LazyTNetstring::InvalidScope)
        end
      end

      context "when updating a key to nil" do
        let(:empty)     { TNetstring.dump({}) }

        it "should delete the key" do
          subject[key] = nil
          subject.data.should == empty
        end
      end

      context "when updating a non-existing key to an empty hash" do
        let(:data)      { TNetstring.dump({}) }
        let(:key)       { 'key' }
        let(:value)     { 'value' }
        let(:new_data)  { TNetstring.dump({ key => value }) }

        it "should add a new key value pair" do
          subject[key] = value
          subject.data[0..1].should == "14"
          subject.data.should == new_data
        end
      end

      context "when updating a non-existing key to a non-empty hash" do
        let(:data)      { TNetstring.dump({ 'some-key' => 'some-value' }) }
        let(:key)       { 'key' }
        let(:value)     { 'value' }
        let(:new_data)  { TNetstring.dump({ 'some-key' => 'some-value', key => value }) }

        it "should add a new key at the end of the data store" do
          subject[key] = value
          subject.data.should == new_data
        end
      end

      context 'with key mappings' do
        let(:key) { short_key }
        let(:long_key) { 'x' * 100 }
        let(:short_key) { 'x' }
        let(:mappings) { {long_key => short_key} }
        let(:data) { LazyTNetstring.dump({}) }
        let(:expected) { TNetstring.dump({short_key => new_value}) }
        let(:data_access) { LazyTNetstring::DataAccess.new(data, :mappings => mappings) }

        it "should set the value with the mapped key" do
          subject[long_key] = new_value
          subject.data.should == expected
        end
      end
    end

    describe "#delete" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context "when removing terms" do
        let(:empty)     { TNetstring.dump({}) }
        let(:data)      { TNetstring.dump({ key => value }) }
        let(:key)       { 'key' }
        let(:value)     { 'value' }

        it "should do nothing if key doesn't exist" do
          subject.delete('non_existing_key')
          subject.data.should == data
        end
        it "should decrease the parents length" do
          subject.delete(key)
          subject.data.should == empty
        end
        it "should return the deleted value" do
          subject.delete(key).should == value
        end
      end

      context "when removing keys present in different scopes" do
        let(:data)        { TNetstring.dump({
                              'key' => 'value',
                              'key2' => {
                                'key' => 'value',
                                'key2' => 'value2'
                              }
                            })}
        let(:data_inner)  { TNetstring.dump({
                              'key2' => {
                                'key' => 'value',
                                'key2' => 'value2'
                              }
                            })}
        let(:data_outer)  { TNetstring.dump({
                              'key' => 'value',
                              'key2' => {
                                'key2' => 'value2'
                              }
                            })}
        it "should not delete the inner key" do
          subject.delete('key')
          subject.data.should == data_inner
        end
        it "should not delete the outer key" do
          subject['key2'].delete('key')
          subject.data.should == data_outer
        end
      end

      context "when removing terms larger data stores" do
        let(:data)      { TNetstring.dump({ 'key1' => 'value1', 'key2' => 'value2', 'key3' => 'value3' }) }
        let(:new_data)  { TNetstring.dump({ 'key1' => 'value1', 'key3' => 'value3' }) }

        it "should delete the part of TNetstring from the data store" do
          subject.delete('key2')
          subject.data.should == new_data
        end
      end

      context "when removing terms nested data stores" do
        let(:data)      { TNetstring.dump({
                            'level1' => {
                              'level2' => {
                                'level3' => {'key' => 'value' }
                              }
                            }
                          })}
        let(:new_data)  { TNetstring.dump({
                            'level1' => {
                              'level2' => {
                                'level3' => { }
                              }
                            }
                          })}
        it "should update all parent lengths" do
          subject['level1']['level2']['level3'].delete('key')
          subject.data.should == new_data
        end
      end

      context 'with key mappings' do
        let(:long_key) { 'x' * 100 }
        let(:short_key) { 'x' }
        let(:mappings) { {long_key => short_key} }
        let(:data) { TNetstring.dump({short_key => 'value'}) }
        let(:expected) { LazyTNetstring.dump({}) }
        let(:data_access) { LazyTNetstring::DataAccess.new(data, :mappings => mappings) }

        it "should delete the mapped key" do
          subject.delete(long_key)
          subject.data.should == expected
        end
      end
    end

    describe "incrementing and decrementing" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context "when incerementing" do
        let(:data) { TNetstring.dump({key => -123}) }
        let(:key)  { "key" }

        it "should increment value by one" do
          subject.increment_value(key)
          subject[key].should == -122
        end
      end

      context "when decrementing" do
        let(:data) { TNetstring.dump({key => -123}) }
        let(:key)  { "key" }

        it "should decrement value by one" do
          subject.decrement_value(key)
          subject[key].should == -124
        end
      end

      context "when incerementing a non existing key" do
        let(:data) { TNetstring.dump({}) }
        let(:key)  { "key" }

        it "should increment value by one" do
          subject.increment_value(key)
          subject[key].should == 1
        end

        it "should decrement value by one" do
          subject.decrement_value(key)
          subject[key].should == -1
        end
      end
    end

    describe "more complex test cases" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context "when setting a deeply nested key" do
        let(:data)      { TNetstring.dump({
                            'front' => 'padding',
                            'level1' => {
                              'front' => 'padding',
                              'level2' => {
                                'front' => 'padding',
                                'level3' => { key => old_value },
                                'after' => { 'key' => 'value' },
                                'end' => 'padding'
                              },
                              'after' => { 'key' => 'value' },
                              'end' => 'padding'
                            },
                            'after' => { 'key' => 'value' },
                            'end' => 'padding'
                          })
                        }
        let(:new_data)  { TNetstring.dump({
                            'front' => 'padding',
                            'level1' => {
                              'front' => 'padding',
                              'level2' => {
                                'front' => 'padding',
                                'level3' => { key => new_value },
                                'after' => { 'key' => 'value' },
                                'end' => 'padding'
                              },
                              'after' => { 'key' => 'value' },
                              'end' => 'padding'
                            },
                            'after' => { 'key' => 'value' },
                            'end' => 'padding'
                          })
                        }
        let(:old_value) { [] }
        let(:new_value) { 'a' * 10000 }
        let(:key)       { 'magic' }

        it "should update the nested value and set correct offsets" do
          level1 = subject['level1']
          after = subject['after']
          after1 = level1['after']
          level2 = level1['level2']
          after2 = level2['after']
          level3 = level2['level3']
          level3[key] = new_value
          subject.data.should == new_data
          subject.offset.should == 0
          level1.offset.should == 33
          level2.offset.should == 66
          level3.offset.should == 99
          after.offset.should == 10215
          after1.offset.should == 10172
          after2.offset.should == 10129
        end
      end

      context "when setting a key to itself" do
        let(:data)      { TNetstring.dump({ key => 'x' * 100 }) }
        let(:key)       { 'key' }

        it "should work" do
          subject[key] = subject[key]
          subject.data.should == data
        end
      end

      context "when copying a value to another key" do
        let(:data)      { TNetstring.dump({ key => value }) }
        let(:new_data)  { TNetstring.dump({ key => value, new_key => value }) }
        let(:value)     { 'x' * 100 }
        let(:key)       { 'key' }
        let(:new_key)   { 'new_key' }

        it "should not affect the old key" do
          subject[new_key] = subject[key]
          subject.data.should == new_data
        end
      end

      context "when garbage collecting" do
        let(:data)      { TNetstring.dump({ 'inner' => { key => value }}) }
        let(:value)     { 'x' * 100 }
        let(:key)       { 'key' }

        it "should not not delete the parent of a referenced child" do
          parent = LazyTNetstring::DataAccess.new data
          child = parent['inner']
          parent = nil
          # Invoke GC a few times and try to mess up the memory
          (1..100).each { GC.start }
          (1..100).each { |i| LazyTNetstring::DataAccess.new TNetstring.dump({'key' * i => 'value' * i}) }
          child.data.should == data
        end
        it "should not delete children with a live reference" do
          parent = LazyTNetstring::DataAccess.new data
          child = parent['inner']
          child2 = parent['inner']
          child = nil
          # Invoke GC a few times and try to mess up the memory
          (1..100).each { GC.start }
          (1..100).each { |i| LazyTNetstring::DataAccess.new TNetstring.dump({'key' * i => 'value' * i}) }
          child2.data.should == data
        end
      end

      context "when updating a short nested key" do
        let(:inner)   { { 'key' => old_value } }
        let(:data)    { TNetstring.dump({
                          'first' => inner,
                          'second' => inner.merge({'nested' => inner }),
                          'third' => inner
                        })
                      }
        let(:old_value) { 'x' }
        let(:new_value) { 'x' * 100 }
        
        it "should update offsets for children correctly" do
          first = subject['first']
          second = subject['second']
          nested = second['nested']
          third = subject['third']
          second['key'] = new_value
          first.scoped_data.should == TNetstring.dump(inner)
          third.scoped_data.should == TNetstring.dump(inner)
          nested.offset.should > second.offset
          nested.scoped_data.should == TNetstring.dump(inner)
        end
      end
      context "when updating a long nested key" do
        let(:inner)   { { 'key' => old_value } }
        let(:data)    { TNetstring.dump({
                          'first' => inner,
                          'second' => inner.merge({'nested' => inner}),
                          'third' => inner
                        })
                      }
        let(:old_value) { 'x' * 100 }
        let(:new_value) { 'x' }
        
        it "should update offsets for children correctly" do
          first = subject['first']
          second = subject['second']
          nested = second['nested']
          third = subject['third']
          second['key'] = new_value
          first.scoped_data.should == TNetstring.dump(inner)
          third.scoped_data.should == TNetstring.dump(inner)
          nested.offset.should > second.offset
          nested.scoped_data.should == TNetstring.dump(inner)
        end
      end

      context "when removing a cached hash before another cached hash" do
        let(:inner)   { { 'key' => 'value' } }
        let(:data)    { TNetstring.dump({
                          'first' => inner,
                          'second' => inner 
                        })
                      }
        it "should update the offset of the trailing child" do
          first = subject['first']
          second = subject['second']
          subject.delete('first')
          second.scoped_data.should == TNetstring.dump(inner)
          second.offset.should == 12
        end
        it "should raise InvalidScope when using the delete hash" do
          first = subject['first']
          subject.delete('first')
          expect { first['key'] }.to raise_error(LazyTNetstring::InvalidScope)
          expect { first['key'] = 'new' }.to raise_error(LazyTNetstring::InvalidScope)
          expect { first.delete('key') }.to raise_error(LazyTNetstring::InvalidScope)
          expect { first.data }.to raise_error(LazyTNetstring::InvalidScope)
          expect { first.scoped_data }.to raise_error(LazyTNetstring::InvalidScope)
        end
      end

      context "when the prefix length of nested hash changes but not on the top-level" do
        let(:inner)   { { 'key' => old_value } }
        let(:data)    { TNetstring.dump({
                          'key' => 'padding' * 100,
                          'first' => inner,
                          'second' => inner 
                        })
                      }
        let(:old_value) { 'x' }
        let(:new_value) { 'x' * 100 }
        it "should update the offset of the trailing child" do
          first = subject['first']
          second = subject['second']
          old_offset = second.offset
          first['key'] = new_value
          second.scoped_data.should == TNetstring.dump(inner)
          second.offset.should == 848
        end
      end

      context "when parsing a string containing null bytes" do
        let(:data)          { TNetstring.dump({ key => null_string}) }
        let(:null_string)   { "\000aaa\000\000\000aaa\000" }
        let(:key)           { "key" }

        it "should not truncate the payload" do
          subject.data.should == data;
          subject[key].should == null_string
        end
      end
    end

    describe "#empty?" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context "when creating a empty hash" do
        let(:data) { TNetstring.dump({}) }

        it "should be empty" do
          subject.empty?.should == true
        end
      end

      context "when getting a nested a empty hash" do
        let(:data) { TNetstring.dump({ key => {} }) }
        let(:key)  { 'key' }

        it "should return true for the nested" do
          subject[key].empty?.should == true
        end
        it "should return false for the outer hash" do
          subject.empty?.should == false
        end
      end
    end

    describe "#each" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context "for each on empty hash" do
        let(:data) { TNetstring.dump({}) }

        it "should be empty" do
          subject.each { "should never enter block".should == nil }
        end
        it "should throw error if no block is given" do
            expect { subject.each }.to raise_error(ArgumentError)
        end
      end

      context "for each on non-empty hash" do
        let(:data) { TNetstring.dump({'key1' => 1, 'key2' => 2, 'key3' => 3}) }

        it "should iterate over all key-value pairs" do
          yields = 0
          subject.each do |key, value|
            yields += 1
            key.should == "key#{value}"
          end
          yields.should == 3
        end
      end

      context "when changing data during iteration" do
        let(:data) { TNetstring.dump({'key1' => 1, 'key2' => 2, 'key3' => 3}) }
        it "should should raise an exception" do
          expect {
            subject.each do |key, value|
              subject[key] = value * 10
            end
          }.to raise_error(LazyTNetstring::InvalidTNetString)
        end
      end

      context "when adding keys during nested iteration" do
        let(:data)           { TNetstring.dump({'key1' => 1, 'inner' => {'key2' => 2, 'key3' => 3}}) }
        let(:expected_data)  { TNetstring.dump(Hash[(2..100).collect {|v| ["key#{v}", v]}]) }
        it "should be ok" do
          subject['inner'].each do |key, value|
            subject['key1'] = 'x' * value
            new_value = value < 100 ? value + 1 : 100
            subject['inner']["key#{new_value}"] = new_value
          end
          subject['inner'].scoped_data.should == expected_data
        end
      end

      context 'with key mappings' do
        let(:data)      { TNetstring.dump({'key' => 'value'}) }
        let(:expected)  { ['long-key-name', 'value'] }
        let(:mappings)  { { 'long-key-name' => 'key' } }
        let(:data_access) { LazyTNetstring::DataAccess.new(data, :mappings => mappings) }

        it "should yield a key with the name from the mappings" do
          result = nil
          subject.each { |args| result = args }
          result.should == expected
        end
      end
    end

    describe "#to_hash" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context "for empty data access" do
        let(:data)      { TNetstring.dump(expected) }
        let(:expected)  { {} }

        it "should return a empty hash" do
          subject.to_hash.should == expected
        end
      end

      context "for non-empty data access" do
        let(:data)      { TNetstring.dump(expected) }
        let(:expected)  { { 'key' => 'value' } }

        it "should return a correct hash" do
          subject.to_hash.should == expected
        end
      end

      context "for data access containing a nested hash" do
        let(:data)      { TNetstring.dump(expected) }
        let(:expected)  { { 'outer' => inner } }
        let(:inner)     { { 'key' => 'value' } }

        it "should return a correct hash with a nested data access" do
          subject.to_hash['outer'].data.should == data
          subject.to_hash['outer'].scoped_data.should == TNetstring.dump(inner)
        end
      end

      context 'with key mappings' do
        let(:data)      { TNetstring.dump({'key' => 'value'}) }
        let(:expected)  { { 'long-key-name' => 'value' } }
        let(:mappings)  { { 'long-key-name' => 'key' } }
        let(:data_access) { LazyTNetstring::DataAccess.new(data, :mappings => mappings) }

        it "should return a hash with the names from the mappings" do
          subject.to_hash.should == expected
        end
      end
    end

    describe "#dup" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context "when duplicating a simple data access" do
        let (:data)  { TNetstring.dump({ 'key' => 'value' }) }

        it "should create a correct copy" do
          copy = subject.dup
          copy.data.should == subject.data
        end
        it "should copy the underlying data" do
          copy = subject.dup
          copy.delete('key')
          copy['key'].should be_nil
          subject['key'].should == 'value'
        end
        it "should not free the data when original gets GC'd" do
          orig = LazyTNetstring::DataAccess.new(data)
          copy = orig.dup
          orig = nil
          (1..100).each { GC.start }
          copy.data.should == data
        end
      end

      context "when duplicating a nested data access" do
        let(:data)  { TNetstring.dump({ 'outer' => inner }) }
        let(:inner) { { 'key' => 'value' } }

        it "should raise exception" do
          expect { subject['outer'].dup }.to raise_error(TypeError)
        end
      end

      context 'when duplicating a data access with a key mapping' do
        let(:data)  { TNetstring.dump({ 'key' => 'value' }) }
        let(:mappings) { {'long-key' => 'key'} }
        let(:data_access) { LazyTNetstring::DataAccess.new(data, :mappings => mappings) }

        it "should create a copy with the same key mappings" do
          copy = subject.dup
          copy['long-key'].should == subject['long-key']
        end
      end
    end

    describe "key mapping scheme" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data, :mappings => mapping) }

      context "when updating" do
        let(:data)     { TNetstring.dump({ "f" => "bar"}) }
        let(:new_data) { TNetstring.dump({ "f" => "baz"}) }
        let(:mapping)  { { "foo" => "f" } }

        it "should change the existing value for the mapped key" do
          subject['foo'] = "baz"
          subject.data.should == new_data
        end
      end

      context "when adding" do
        let(:data)     { TNetstring.dump({})}
        let(:new_data) { TNetstring.dump({ "f" => "bar"}) }
        let(:mapping)  { { "foo" => "f" } }

        it "should add the smaller key" do
          subject['foo'] = "bar"
          subject.data.should == new_data
        end
      end

      context "when deleteing" do
        let(:data)     { TNetstring.dump({ "f" => "bar"}) }
        let(:new_data) { TNetstring.dump({}) }
        let(:mapping)  { { "foo" => "f" } }

        it "should delete the smaller key" do
          subject.delete('foo')
          subject.data.should == new_data
        end
      end

      context "when updating a nested hash" do
        let(:data)     { TNetstring.dump({ "outer" => { "f" => "bar" } }) }
        let(:new_data) { TNetstring.dump({ "outer" => { "f" => "baz" } }) }
        let(:mapping)  { { "foo" => "f" } }

        it "should change the exiting value for the smaller key" do
          subject[ "outer" ][ "foo" ] = "baz"
          subject.data.should == new_data
        end
      end
    end

  end
end
