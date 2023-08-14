set dir [file dirname [info script]]
load [file join $dir build libtjson.so]

set json_spec [::tjson::parse {{"a":1,"b":2,"c":"hello\nworld", "flag": true}}]
puts json_spec=$json_spec
puts json=[::tjson::to_json $json_spec]

puts [::tjson::parse {{"a":1,"b":2,"c":"hello"}} true]

puts list=[::tjson::parse {[1,2,3]}]
