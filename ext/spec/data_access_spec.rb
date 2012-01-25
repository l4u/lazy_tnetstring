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

  end
end
