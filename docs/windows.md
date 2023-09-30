# tjson - Installation Guide for Windows

## Prerequisites

In order to compile the tjson extension for Windows, you need the following:

- [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)

## Build TCL

Open "Developer Powershell for VS 2022" as Administrator
and run the following commands:

```

Build TCL:

```bash
# Download http://prdownloads.sourceforge.net/tcl/tcl8613-src.zip
# and extract to a directory of your choice
cd tcl8.6.13/win
nmake -f makefile.vc
nmake -f makefile.vc install
```

Build tjson extension on windows:

```
# Download tjson source code and unzip it to a directory of your choice.
cd c:/path/to/tjson
mkdir build
cd build
# If you want to build for 64-bit, use -A x64 instead of -A Win32.
# but note that TCL figured out the compiler architecture
# and version automatically when it was built in the previous step.
cmake .. \
  -A Win32 \
  -DTCL_LIBRARY_DIR="C:/Tcl/lib" \
  -DTCL_INCLUDE_DIR="C:/Tcl/include" \
  -DTCL_LIBRARY="C:/Tcl/lib/tcl86t.lib"
cmake --build . --config=Release
cmake --install . --config=Release
```
