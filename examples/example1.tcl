package require tjson

set json_spec [::tjson::json_to_typed {{"a":1,"b":2,"c":"hello\nworld", "flag": true}}]
puts json_spec=$json_spec
puts json=[::tjson::typed_to_json $json_spec]

puts json_to_simple=[::tjson::json_to_simple {{"a":1,"b":2,"c":"hello"}}]

puts list=[::tjson::json_to_typed {[1,2,3]}]
puts json_to_typed=[::tjson::json_to_typed {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}}]
puts json_to_simple=[::tjson::json_to_simple {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}}]
puts typed_to_json=[::tjson::typed_to_json {M {a {N 1} b {BOOL 1} c {L {{N 1} {N 2} {N 3}}} d {M {d1 {S a} d2 {S b}}} e {BOOL 0} f {N 123.45}}}]
puts escape_json_string=[::tjson::escape_json_string "hello\"world\n"]

