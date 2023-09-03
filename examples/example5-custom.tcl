package require tjson

set doc_example [::tjson::custom_to_typed {{mydoc} document {{a int32 1} {b int32 2} {c s "hello\nworld"} {flag b true}}}]
puts doc_example,typed=$doc_example
puts doc_example,json=[::tjson::typed_to_json $doc_example]

set arr_example [::tjson::custom_to_typed {{myarr} a {{a int32 1} {b int32 2} {c s "hello\nworld"} {flag b true}}}]
puts arr_example,typed=$arr_example
puts arr_example,json=[::tjson::typed_to_json $arr_example]

