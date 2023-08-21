set dir [file dirname [info script]]
load [file join $dir build libtjson.so]

set json_spec [::tjson::parse {{"a":1,"b":2,"c":"hello\nworld", "flag": true}}]
puts json_spec=$json_spec
puts json=[::tjson::to_json $json_spec]

puts [::tjson::parse {{"a":1,"b":2,"c":"hello"}} true]

puts list=[::tjson::parse {[1,2,3]}]


puts [::tjson::parse {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}}]
puts [::tjson::parse {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}} true]

puts [::tjson::to_json {M {a {N 1} b {BOOL 1} c {L {{N 1} {N 2} {N 3}}} d {M {d1 {S a} d2 {S b}}}}}]

puts [::tjson::escape_json_string "hello\"world\n"]

