# tjson

TCL/C extension for parsing JSON.

## Tcl Commands
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
git clone --recurse-submodules https://github.com/jerily/tjson.git
cd tjson
TJSON_DIR=$(pwd)
```

## Build and install the dependencies
```bash
cd ${TJSON_DIR}/cJSON
mkdir build
cd build
cmake ..
make
make install
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

