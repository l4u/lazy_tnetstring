require 'spec_helper'
require 'tnetstring'

module LazyTNetstring
  describe DataAccess do

    describe '#new' do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

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
    end

    describe '#[]' do
      subject   { LazyTNetstring::DataAccess.new(data)[key]}
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
    end

    describe '#[]=(key, new_value)' do
      subject         { LazyTNetstring::DataAccess.new(data) }
      let(:data)      { TNetstring.dump({key => old_value}) }
      let(:key)       { 'foo' }
      let(:old_value) { 'bar' }
      let(:new_value) { 'baz' }

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

        it "should remove the key" do
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
    end

    describe "#remove" do
      subject           { data_access }
      let(:data_access) { LazyTNetstring::DataAccess.new(data) }

      context "when removing terms" do
        let(:empty)     { TNetstring.dump({}) }
        let(:data)      { TNetstring.dump({ key => value }) }
        let(:key)       { 'key' }
        let(:value)     { 'value' }

        it "should do nothing if key doesn't exist" do
          subject.remove('non_existing_key')
          subject.data.should == data
        end
        it "should decrease the parents length" do
          subject.remove(key)
          subject.data.should == empty
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
        it "should not remove the inner key" do
          subject.remove('key')
          subject.data.should == data_inner
        end
        it "should not remove the outer key" do
          subject['key2'].remove('key')
          subject.data.should == data_outer
        end
      end

      context "when removing terms larger data stores" do
        let(:data)      { TNetstring.dump({ 'key1' => 'value1', 'key2' => 'value2', 'key3' => 'value3' }) }
        let(:new_data)  { TNetstring.dump({ 'key1' => 'value1', 'key3' => 'value3' }) }

        it "should remove the part of TNetstring from the data store" do
          subject.remove('key2')
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
          subject['level1']['level2']['level3'].remove('key')
          subject.data.should == new_data
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
          subject['level1']['level2']['level3'][key] = new_value
          subject.data.should == new_data
          subject.offset.should == 0
          subject['level1'].offset.should == 33
          subject['level1']['level2'].offset.should == 66
          subject['level1']['level2']['level3'].offset.should == 99
          subject['after'].offset.should == 10215
          subject['level1']['after'].offset.should == 10172
          subject['level1']['level2']['after'].offset.should == 10129
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

  end
end
