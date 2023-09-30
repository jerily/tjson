# tjson - Installation Guide for Windows



## Prerequisites

In order to compile the tjson extension for Windows, you need the following:

- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)

## Win32

Build TCL:

```bash
# Download http://prdownloads.sourceforge.net/tcl/tcl8613-src.zip
# and extract to a directory of your choice
cd tcl8.6.13/win
nmake -f makefile.vc
nmake -f makefile.vc install
```

Build tjson extension on windows:

```bash
cd ${TJSON_DIR}
mkdir build
cd build
cmake .. \
  -A Win32 \
  -DTCL_LIBRARY_DIR="C:/Tcl/lib" \
  -DTCL_INCLUDE_DIR="C:/Tcl/include" \
  -DTCL_LIBRARY="C:/Tcl/lib/tcl86t.lib"
cmake --build . --config=Release
cmake --install . --config=Release
```
