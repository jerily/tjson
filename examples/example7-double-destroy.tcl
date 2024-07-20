package require tjson

# object manipulation commands
::tjson::parse {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}} node_handle
set item_handle [::tjson::get_object_item $node_handle d]
if { [catch {::tjson::destroy $item_handle} errmsg] } {
    puts "Expected Error: $errmsg"
}
puts hello
::tjson::destroy $node_handle