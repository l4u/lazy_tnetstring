require 'spec_helper'
# Copyright (c) 2011 Matt Yoho (MIT-LICENSE)

describe LazyTNetstring do
  context "parsing" do
    context "integers" do
      it "parses a positive integer" do
        LazyTNetstring.parse('5:12345#').should == 12345
      end

      it "parses a negative integer" do
        LazyTNetstring.parse('6:-12345#').should == -12345
      end
    end

    context "floats" do
      it "parses a positve float" do
        LazyTNetstring.parse('3:3.5^').should == 3.5
      end

      it "parses a negative float" do
        LazyTNetstring.parse('5:-3.14^').should == -3.14
      end

      it "parses a float with leading zeros" do
        LazyTNetstring.parse('7:-000.14^').should == -0.14
      end
    end

    it "parses an empty string" do
      LazyTNetstring.parse('0:,').should == ""
    end

    it "parses a string" do
      LazyTNetstring.parse('12:this is cool,').should == "this is cool"
    end

    it "parses to an empty array" do
      LazyTNetstring.parse('0:]').should == []
    end

    it "parses an arbitrary array of ints and strings" do
      LazyTNetstring.parse('24:5:12345#5:67890#5:xxxxx,]').should == [12345, 67890, 'xxxxx']
    end

    it "parses to an empty hash" do
      LazyTNetstring.parse('0:}').to_hash.should == {}
    end

    it "parses an arbitrary hash of ints, strings, and arrays" do
      LazyTNetstring.parse('34:5:hello,22:11:12345678901#4:this,]}').to_hash.should == {"hello" => [12345678901, 'this']}
    end

    it "parses a null" do
      LazyTNetstring.parse('0:~').should == nil
    end

    it "parses a dictionary with a null value" do
      LazyTNetstring.parse("9:3:key,0:~}").to_hash.should == {"key" => nil}
    end

    it "raises on a lengthy null" do
      expect { LazyTNetstring.parse('1:x~') }.to raise_error(LazyTNetstring::InvalidTNetString)
    end

    it "parses a boolean" do
      LazyTNetstring.parse('4:true!').should == true
    end

    it "raises on a bad boolean" do
      expect { LazyTNetstring.parse('5:pants!') }.to raise_error(LazyTNetstring::InvalidTNetString)
    end

    it "raises with negative length" do
      expect { LazyTNetstring.parse("-1:asd,") }.to raise_error(LazyTNetstring::InvalidTNetString)
    end

    it "raises with absurd length" do
      expect { LazyTNetstring.parse("1000000000:asd,") }.to raise_error(LazyTNetstring::InvalidTNetString)
    end

    it "raises on unknown type" do
      expect { LazyTNetstring.parse('0:)') }.to raise_error(LazyTNetstring::InvalidTNetString)
    end
  end

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

    context "data access" do
      it "dumps an empty hash" do
        tnetstring = '0:}'
        LazyTNetstring.dump(LazyTNetstring::DataAccess.new(tnetstring)).should == tnetstring
      end

      it "dumps an arbitrary hash of primitives and arrays" do
        tnetstring = '34:5:hello,22:11:12345678901#4:this,]}'
        LazyTNetstring.dump(LazyTNetstring::DataAccess.new(tnetstring)).should == tnetstring
      end

      it "dumps nested hashes" do
        tnetstring = '25:5:hello,13:5:world,2:42#}}'
        LazyTNetstring.dump(LazyTNetstring::DataAccess.new(tnetstring)).should == tnetstring
      end

      it "accepts symbols as keys" do
        tnetstring = '25:5:hello,13:5:world,2:24#}}'
        LazyTNetstring.dump(LazyTNetstring::DataAccess.new(tnetstring)).should == tnetstring
      end
    end

    it "rejects non-primitives" do
      expect { LazyTNetstring.dump(Object.new) }.to raise_error(ArgumentError)
    end
  end
end
