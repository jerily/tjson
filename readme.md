# tjson

## JSON Lines (1.0.29)

`::tjson::jsonl_gets ?-simple|-handle? channel varName` reads one JSON value from a blocking
channel into `varName`. It returns 1 for a record (including
`null`) and 0 at EOF. A blank, malformed, or oversized line raises
`{TJSON STRICT}`. The reader consumes that line and leaves the variable
untouched, as it does at EOF. It accepts LF, CRLF, and a final record without a newline.

| Read mode | Value assigned to `varName` | Example using [events.jsonl](examples/events.jsonl) |
| --- | --- | --- |
| Default | Typed Tcl value, suitable for `jsonl_puts`. JSON null is `{N null}`. | [Convert a typed record with typed_to_simple](examples/example9-jsonl.tcl) |
| `-simple` | Tcl dictionary, list, or scalar. Both JSON null and an empty string become an empty Tcl string. | [Read fields with dict get](examples/example10-jsonl-simple.tcl) |
| `-handle` | An independently owned root handle for querying or modifying the parsed record. | [Query each record with JSONPath](examples/example11-jsonl-handle.tcl) |

With `-handle`, call `::tjson::destroy $root` for each root, typically in a
`finally` block inside the loop. Reading another record or unsetting the
variable leaves the previous root alive. Destroying the root also invalidates
its child handles.

`::tjson::jsonl_puts channel typedValue` writes one compact JSON value and a
newline and returns the empty string. Both commands use UTF-8 and limit each record to 1 MiB and 32 levels of nesting.
Use binary translation for exact on-disk framing. The caller owns the channel
and controls flushing and closing.

Run any example from the repository root, for instance
`tclsh examples/example10-jsonl-simple.tcl`.

```tcl
set ch [open events.jsonl rb]
try {
    while {[::tjson::jsonl_gets $ch event]} {
        ::tjson::jsonl_puts stdout $event
    }
} finally {
    close $ch
}
```

## Typed JSON parsing (1.0.30)

`::tjson::json_to_typed ?-maxbytes bytes? ?-maxdepth levels? json` parses one
complete JSON value. It preserves null as `{N null}` and rejects duplicate
object keys (including escaped aliases), malformed JSON, invalid Unicode, and
trailing data. Document errors have code `{TJSON STRICT}` and omit input values.

By default the input can be as large as a Tcl string. The default maximum depth
is cJSON’s nesting limit (1000 in this build). Each enclosing object or array,
including an empty one, counts as one level. Both options take positive integers.
For example, to limit a protocol message to 1 MiB of UTF-8 and 32 levels, use
`::tjson::json_to_typed -maxbytes 1048576 -maxdepth 32 $message`.

cJSON uses NUL-terminated strings and double-precision numbers. The parser
rejects NUL in strings (including `\u0000`), non-finite numeric results, and
number tokens longer than 63 characters. Large integers may lose precision.

The older `::tjson::json_to_typed_strict json ?maximumDepth?` command remains
available with its 1 MiB limit and default depth of 32 (configurable from 1 to 128).

TCL/C extension for parsing, manipulating, and querying JSON.

## Examples
```
package require tjson

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

The following works for Linux and MacOS. For windows, see the [Windows Build Instructions](docs/windows.md).

```bash
For TCL:
```bash
# Build the TCL extension
wget https://github.com/jerily/tjson/archive/refs/tags/v1.0.25.tar.gz
tar -xzf v1.0.25.tar.gz
export TJSON_DIR=$(pwd)/tjson-1.0.25
cd ${TJSON_DIR}
mkdir build
cd build
cmake ..
# or if TCL is not in the default path (/usr/local/lib):
# change "TCL_LIBRARY_DIR" and "TCL_INCLUDE_DIR" to the correct paths
# cmake .. -DTCL_LIBRARY_DIR=/usr/local/lib -DTCL_INCLUDE_DIR=/usr/local/include
cmake --build .
cmake --install .
# tclsh8.6 ../examples/example1.tcl
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
* **::tjson::json_to_typed** *?-maxbytes bytes? ?-maxdepth levels? json_string*
    - returns a typed TCL structure (pairs of types and values, M for object, L for list, S for string, N for number, BOOL for boolean)
* **::tjson::typed_to_simple** *typedValue*
    - Recursively converts a typed Tcl value directly to the same simple representation used by `jsonl_gets -simple`. Objects become dictionaries, arrays become lists, and `{N null}` becomes an empty string. For example, `::tjson::typed_to_simple {M {event {S signup} user {N 42}}}` returns `event signup user 42`.
* **::tjson::typed_to_json** *typed_spec*
    - returns a JSON string from a typed TCL structure (like the one returned by ::tjson::json_to_typed)
* **::tjson::parse** *json_string* *?varname?*
    - returns a handle to manipulate the JSON string
* **::tjson::create** *typed_spec* *?varname?*
    - returns a handle to manipulate the JSON of the typed TCL structure
* **::tjson::destroy** *handle*
    - destroys the JSON node structure for the given handle
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
* **::tjson::get_child_items** *handle*
  - gets all the child items of an object or array 
* **::tjson::get_string** *handle*
    - returns the string key of the node
* **::tjson::get_valuestring** *handle*
    - returns the string value of the node
* **::tjson::is_number** *handle*
  - returns true if node holds a number
* **::tjson::is_bool** *handle*
  - returns true if node holds a boolean value
* **::tjson::is_string** *handle*
  - returns true if node holds a string value
* **::tjson::is_null** *handle*
  - returns true if node holds a nullß
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

## Security

Please see [security.md](security.md) for instructions on reporting vulnerabilities privately.

## Acknowledgements

- [Oz Kaizer (@tatikoma)](https://github.com/tatikoma) for reporting the JSONPath heap buffer overflow and providing a reproducer, patch, and regression test.
- [@mtmcp](https://github.com/mtmcp) for the pull request improving Tcl stub library linking and discovery, static builds, and initialization error handling.
