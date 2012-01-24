require 'spec_helper'

describe LazyTNetstring do
  context "dumping" do
    context "integers" do
      it "dumps a positive integer" do
        LazyTNetstring.dump(42).should == "2:42#"
      end

      it "dumps a negative integer" do
        LazyTNetstring.dump(-42).should == "3:-42#"
      end
    end

    context "floats" do
      it "dumps a positive float" do
        LazyTNetstring.dump(12.3).should == "6:12.300^"
      end

      it "dumps a negative float" do
        LazyTNetstring.dump(-2.3).should == "6:-2.300^"
      end

      it "dumps a float with integral value" do
        LazyTNetstring.dump(-42.0).should == "7:-42.000^"
      end
    end

    it "dumps a string" do
      LazyTNetstring.dump("hello world").should == "11:hello world,"
    end

    context "boolean" do
      it "dumps true as 'true'" do
        LazyTNetstring.dump(true).should == "4:true!"
      end

      it "dumps false as 'false'" do
        LazyTNetstring.dump(false).should == "5:false!"
      end
    end

    it "dumps nil" do
      LazyTNetstring.dump(nil).should == "0:~"
    end

    context "arrays" do
      it "dumps an empty array" do
        LazyTNetstring.dump([]).should == "0:]"
      end

      it "dumps an array of arbitrary elements" do
        LazyTNetstring.dump(["cat", false, 123]).should == "20:3:cat,5:false!3:123#]"
      end

      it "dumps nested arrays" do
        LazyTNetstring.dump(["cat", [false, 123]]).should == "24:3:cat,14:5:false!3:123#]]"
      end
    end

    context "hashes" do
      it "dumps an empty hash" do
        LazyTNetstring.dump({}).should == "0:}"
      end

      it "dumps an arbitrary hash of primitives and arrays" do
        LazyTNetstring.dump({"hello" => [12345678901, 'this']}).should == '34:5:hello,22:11:12345678901#4:this,]}'
      end

      it "dumps nested hashes" do
        LazyTNetstring.dump({"hello" => {"world" => 42}}).should == '25:5:hello,13:5:world,2:42#}}'
      end

      it "accepts symbols as keys" do
        LazyTNetstring.dump({ :hello => {"world" => 24}}).should == '25:5:hello,13:5:world,2:24#}}'
      end

      it "rejects non-String keys" do
        expect { LazyTNetstring.dump({123 => "456"}) }.to raise_error(ArgumentError)
      end
    end

    it "rejects non-primitives" do
      expect { LazyTNetstring.dump(Object.new) }.to raise_error(ArgumentError)
    end
  end
end
