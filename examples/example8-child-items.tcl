package require tjson

::tjson::parse {
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
}} node_handle

set store_handle [::tjson::get_object_item $node_handle store]
set book_handle [::tjson::get_object_item $store_handle book]

foreach item_handle [::tjson::get_child_items $book_handle] {
    puts [::tjson::to_typed $item_handle]
}
