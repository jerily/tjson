# tjson

TCL/C extension for parsing, manipulating, and querying JSON.

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

::tjson::escape_json_string "hello\"world\n"
=> hello\"world\n
```

## Build the tjson extension
For TCL:
```bash
# Build the TCL extension
wget https://github.com/jerily/tjson/archive/refs/tags/v1.0.8.tar.gz
tar -xzf v1.0.8.tar.gz
export TJSON_DIR=$(pwd)/tjson-1.0.8
cd ${TJSON_DIR}
mkdir build
cd build
# change "TCL_LIBRARY_DIR" and "TCL_INCLUDE_DIR" to the correct paths
cmake .. \
  -DTCL_LIBRARY_DIR=/usr/local/lib \
  -DTCL_INCLUDE_DIR=/usr/local/include
make
make install
tclsh ../examples/example1.tcl
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
* **::tjson::query** *handle* *jsonpath*
  - returns a list of handles for the given JSON path expression
* **::tjson::custom_to_typed** *custom_spec*
  - returns a typed TCL structure for the given custom (triple notation / bson) spec
* **::tjson::typed_to_custom** *typed_spec*
  - returns a custom (triple notation / bson) spec from a typed TCL structure (like the one returned by ::tjson::custom_to_typed)


## Typed TCL Notation/Spec

| JSON Type | Spec Type | Example                                                                   |
|-----------|-----------|---------------------------------------------------------------------------|
| Object    | M         | {M {a {N 1} b {BOOL 1} c {L {{N 1} {N 2} {N 3}}}} |
| Array     | L         | {L {{N 1} {N 2} {N 3}}}<br/>{L {{S "this"} {S "is"} {S "a"} {S "test"}}}      |
| String    | S         | {S a}                                                                     |
| Number    | N         | {N 1}                                                                     |
| Boolean   | BOOL      | {BOOL 1}                                                                  |


## JSONPath Syntax

| JSONPath Expression | Description                                                                                               |
|---------------------|-----------------------------------------------------------------------------------------------------------|
| $                   | The root object or array                                                                                  |
| .property           | Selects the specified property in a parent object                                                         |
| ['property']        | Selects the specified property in a parent object. Be sure to put single quotes around the property name. |
| [n]                 | Selects the *n*-th element from an array. Indexes are 0-based.                                            |
| [start:end]         | Selects array elements from the start index and up to, but not including, end index. |
| [start:]            | Selects array elements from the start index to the end of the array. |
| [:end]              | Selects array elements from the first element up to, but not including, the end index. |
| [-n:]               | Selects the last *n* elements in the array. |









