package require tjson

puts [::tjson::custom_to_typed {{whatever} document {{a int32 1} {b int32 2} {c s "hello\nworld"} {flag b true}}}]
puts [::tjson::custom_to_typed {{whatever} a {{a int32 1} {b int32 2} {c s "hello\nworld"} {flag b true}}}]