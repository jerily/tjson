# tjson

TCL/C extension for parsing and manipulating JSON.

## Examples
```
# Parse JSON string and return a simple TCL structure
# ::tjson::json_to_simple json_string

::tjson::json_to_simple {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}} true
=> a 1 b 1 c {1 2 3} d {d1 a d2 b} 

# Parse JSON string and return a typed TCL structure
# ::tjson::json_to_typed json_string

::tjson::json_to_typed {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}}
=> M {a {N 1} b {BOOL 1} c {L {{N 1} {N 2} {N 3}}} d {M {d1 {S a} d2 {S b}}}}

# Serialize a typed TCL structure to JSON.
# ::tjson::typed_to_json typed_spec

::tjson::typed_to_json {M {a {N 1} b {BOOL 1} c {L {{N 1} {N 2} {N 3}}} d {M {d1 {S a} d2 {S b}}}}}
=> {"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1": "a", "d2": "b"}}

# Escape JSON string
# ::tjson::escape_json_string string

::tjson::escape_json_string "hello\nworld\n"
=> hello\"world\n
```

## Clone the repository
```bash
git clone https://github.com/jerily/tjson.git
cd tjson
TJSON_DIR=$(pwd)
```

## Build the library
For TCL:
```bash
# Build the TCL extension
cd ${TJSON_DIR}
mkdir build
cd build
cmake ..
make
make install
tclsh ../example.tcl
```

For NaviServer using Makefile:
```
cd ${TJSON_DIR}
make
make install
```

## TCL Commands

* **::tjson::json_to_simple** *json_string*
    - returns a simple TCL structure (e.g. list, dict, or string)
* **::tjson::json_to_typed** *json_string*
    - returns a typed TCL structure (pairs of types and values, M for object, L for list, S for string, N for number, BOOL for boolean)
* **::tjson::typed_to_json** *typed_spec*
    - returns a JSON string from a typed TCL structure (like the one returned by ::tjson::json_to_typed)
* **::tjson::parse** *json_string*
    - returns a handle to manipulate the JSON string
* **::tjson::delete** *handle*
    - deletes the JSON node structure for the given handle
* **::tjson::size** *handle*
  - returns the size of the JSON node structure for the given handle
* **::tjson::add_item_to_object** *handle* *key* *typed_spec*
  - adds an item to an object using the typed format
* **::tjson::replace_item_in_object** *handle* *key* *typed_spec*
  - replaces an item in an object using the given typed format
* **::tjson::delete_item_from_object** *handle* *key*
  - deletes an item from an object
* **::tjson::get_object_item** *handle* *key*
  - gets an item from an object
* **::tjson::add_item_to_array** *handle* *typed_spec*
  - adds an item to an array using the typed format
* **::tjson::insert_item_in_array** *handle* *index* *typed_spec*
  - inserts an item at the given 0 based index and shifts all the existing items to the right
* **::tjson::replace_item_in_array** *handle* *index* *typed_spec*
  - replaces an item at the given 0 based index
* **::tjson::delete_item_from_array** *handle* *index*
  - deletes an item at the given 0 based index and shifts all the existing items to the left
* **::tjson::get_array_item** *handle* *index*
  - gets an item at the given 0 based index
* **::tjson::to_simple** *handle*
  - returns a simple TCL structure (e.g. list, dict, or string) for the given node
* **::tjson::to_typed** *handle*
  - returns a typed TCL structure for the given node
* **::tjson::to_json** *handle*
  - returns a JSON string for the given node
* **::tjson::to_pretty_json** *handle*
  - returns a prettified JSON string for the given node






