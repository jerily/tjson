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

set item_handles [::tjson::query $node_handle {$.store..author}]
puts authors=[lmap i $item_handles {::tjson::to_simple $i}]

puts ---------------

set item_handles [::tjson::query $node_handle {$.store.book[0].title}]
puts title=[::tjson::to_simple $item_handles]

puts ---------------

set item_handles [::tjson::query $node_handle {$['store']['book'][2]['title']}]
puts title=[::tjson::to_simple $item_handles]

puts ---------------

set item_handles [::tjson::query $node_handle {$['store'].book[3].title}]
puts title=[::tjson::to_simple $item_handles]

puts ---------------

set item_handles [::tjson::query $node_handle {$['store'].book[1,2,3].title}]
puts titles=[lmap i $item_handles {::tjson::to_simple $i}]

puts ---------------

set item_handles [::tjson::query $node_handle {$['store'].book[1:3].title}]
puts titles=[lmap i $item_handles {::tjson::to_simple $i}]

puts ---------------

set item_handles [::tjson::query $node_handle {$['store'].book[*].title}]
puts titles=[lmap i $item_handles {::tjson::to_simple $i}]

puts ---------------

set item_handles [::tjson::query $node_handle {$['store'].book[*].first_name}]
puts titles=[lmap i $item_handles {::tjson::to_simple $i}]

::tjson::destroy $node_handle
