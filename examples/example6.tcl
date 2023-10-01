package require tjson

proc check_untrace_1 {} {
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
       }
    } store_handle
}

proc check_untrace_2 {} {
    ::tjson::create [list M [list a {S "hello world"} b {N 9}]] sample_handle
}

proc check_write_1 {} {
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
       }
    } store_handle
    if { [catch {
        set store_handle "hello world"
    } errmsg] } {
        puts "expected error (var is read-only): $errmsg"
    }
}

proc check_write_2 {} {
    ::tjson::create [list M [list a {S "hello world"} b {N 9}]] sample_handle
    if { [catch {
        set sample_handle "hello world"
    } errmsg] } {
        puts "expected error (var is read-only): $errmsg"
    }
}

check_untrace_1
check_untrace_2

check_write_1
check_write_2
