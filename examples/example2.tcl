package require tjson

# object manipulation commands
set node_handle [::tjson::parse {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}}]
puts size=[::tjson::size $node_handle]
::tjson::add_item_to_object $node_handle e [list N 4]
::tjson::add_item_to_object $node_handle f [list L [list {N 5} {N 6} {N 7}]]
::tjson::add_item_to_object $node_handle g [list M [list a {S "hello world"} b {N 9}]]
puts simple,after=[::tjson::to_simple $node_handle]
puts json,after=[::tjson::to_json $node_handle]
::tjson::replace_item_in_object $node_handle f [list S "this is a test"]
::tjson::delete_item_from_object $node_handle c
puts pretty_json,after,after=[::tjson::to_pretty_json $node_handle]
set item_handle [::tjson::get_object_item $node_handle d]
puts json,get_object_item,d=[::tjson::to_json $item_handle]
::tjson::destroy $node_handle
if { [catch { puts json,get_object_item,d=[::tjson::to_json $item_handle] } errmsg ] } {
    puts "expected error: $errmsg"
}

# array manipulation commands
set node_handle [::tjson::parse {[1,2,3]}]
puts size,before=[::tjson::size $node_handle]
::tjson::add_item_to_array $node_handle [list N 4]
::tjson::add_item_to_array $node_handle [list L [list {N 5} {N 6} {N 7}]]
::tjson::add_item_to_array $node_handle [list M [list a {S "hello world"} b {N 9}]]
puts size,after=[::tjson::size $node_handle]
puts simple,after=[::tjson::to_simple $node_handle]
puts typed,after=[::tjson::to_typed $node_handle]
::tjson::replace_item_in_array $node_handle 2 [list S "this is a test"]
::tjson::delete_item_from_array $node_handle 1
puts simple,after,after=[::tjson::to_simple $node_handle]
set item_handle [::tjson::get_array_item $node_handle 2]
puts json,get_array_item,2=[::tjson::to_json $item_handle]
::tjson::destroy $node_handle
if { [catch { puts json,get_array_item,d=[::tjson::to_json $item_handle] } errmsg ] } {
    puts "expected error: $errmsg"
}

set node_handle [::tjson::create [list M [list a {S "hello world"} b {N 9}]]]
puts size=[::tjson::size $node_handle]
puts json,created=[::tjson::to_json $node_handle]
::tjson::destroy $node_handle
