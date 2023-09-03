package require tjson

set doc_example [::tjson::custom_to_typed [list mydoc document [list a int32 1 b int32 2 c string "hello\nworld" flag boolean true]]]
puts doc_example,typed=$doc_example
puts doc_example,json=[::tjson::typed_to_json $doc_example]

set arr_example [::tjson::custom_to_typed [list myarr array [list 0 int32 1 1 int32 2 2 string "hello\nworld" 3 boolean true]]]
puts arr_example,typed=$arr_example
puts arr_example,json=[::tjson::typed_to_json $arr_example]

