package require tjson

set doc_typed [::tjson::custom_to_typed [list mydoc document [list a int32 1 b int32 2 c string "hello\nworld" flag boolean true]]]
puts doc_typed=$doc_typed
puts doc_typed_json=[::tjson::typed_to_json $doc_typed]
puts doc_custom=[::tjson::typed_to_custom $doc_typed]

set arr_typed [::tjson::custom_to_typed [list myarr array [list 0 int32 1 1 int32 2 2 string "hello\nworld" 3 boolean true]]]
puts arr_typed=$arr_typed
puts arr_typed_json=[::tjson::typed_to_json $arr_typed]
puts arr_custom=[::tjson::typed_to_custom $arr_typed]
