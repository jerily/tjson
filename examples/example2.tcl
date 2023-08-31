package require tjson

set node_handle [::tjson::parse {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}}]
puts size=[::tjson::size $node_handle]
::tjson::add_item_to_object $node_handle e [list N 4]
::tjson::add_item_to_object $node_handle f [list L [list {N 5} {N 6} {N 7}]]
::tjson::add_item_to_object $node_handle g [list M [list a {S "hello world"} b {N 9}]]
puts simple,after=[::tjson::to_simple $node_handle]
puts json,after=[::tjson::to_json $node_handle]
puts pretty_json,after=[::tjson::to_pretty_json $node_handle]
::tjson::destroy $node_handle

set node_handle [::tjson::parse {[1,2,3]}]
puts size,before=[::tjson::size $node_handle]
::tjson::add_item_to_array $node_handle [list N 4]
::tjson::add_item_to_array $node_handle [list L [list {N 5} {N 6} {N 7}]]
::tjson::add_item_to_array $node_handle [list M [list a {S "hello world"} b {N 9}]]
puts size,after=[::tjson::size $node_handle]
puts simple,after=[::tjson::to_simple $node_handle]
puts typed,after=[::tjson::to_typed $node_handle]
::tjson::destroy $node_handle

set node_handle [::tjson::parse {"hello world"}]
puts size=[::tjson::size $node_handle]
::tjson::destroy $node_handle
