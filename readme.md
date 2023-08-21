# tjson

TCL/C extension for parsing JSON.

## Tcl Commands
```
# Parse JSON string and return a simple TCL dictionary or a typed one.
# ::tjson::parse json_string ?simple?

::tjson::parse {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}}
=> M {a {N 1} b {BOOL 1} c {L {{N 1} {N 2} {N 3}}} d {M {d1 {S a} d2 {S b}}}}

::tjson::parse {{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}} true
=> a 1 b 1 c {1 2 3} d {d1 a d2 b} 

::tjson::parse "{{"a": 1, "b": true, "c": [1, 2, 3], "d": {"d1":"a", "d2":"b"}}" true

# Serialize a typed TCL spec to JSON.
# ::tjson::to_json typed_spec

::tjson::to_json {M {a {N 1} b {BOOL 1} c {L {{N 1} {N 2} {N 3}}} d {M {d1 {S a} d2 {S b}}}}}
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

