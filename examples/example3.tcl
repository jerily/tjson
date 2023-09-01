package require tjson

set node_handle [::tjson::parse {
    { "store": {
       "book": [
         { "category": "reference", "author": "Nigel Rees",
           "title": "Sayings of the Century", "price": "8.95"  },
         { "category": "fiction", "author": "Evelyn Waugh",
           "title": "Sword of Honour", "price": "12.99" },
         { "category": "fiction", "author": "Herman Melville",
           "title": "Moby Dick", "isbn": "0-553-21311-3",
           "price": "8.99" },
         { "category": "fiction", "author": "J. R. R. Tolkien",
           "title": "The Lord of the Rings", "isbn": "0-395-19395-8",
           "price": "22.99" }
       ],
       "bicycle": { "color": "red", "price": "19.95" }
     }
   }}]

set store_handle [::tjson::get_object_item $node_handle store]
::tjson::add_item_to_object $store_handle car [list M \
    [list brand {S "Mercedes"} color {S blue} price {N 19876.57}]]

set book_handle [::tjson::get_object_item $store_handle book]
::tjson::add_item_to_array $book_handle [list M \
    [list \
        category {S "fiction"} \
        author {S "Fyodor Dostoevsky"} \
        title {S "Brothers Karamazov"} \
        isbn {S "0679410031"} \
        price {N "22.19"}]]

puts [::tjson::to_pretty_json $node_handle]
::tjson::destroy $node_handle
