cmake_minimum_required(VERSION 3.22.1)
project(tjson VERSION 1.0.25 LANGUAGES C)

set(TARGET tjson)
set(CMAKE_C_STANDARD   11)
set(CMAKE_C_STANDARD_REQUIRED true)

if(WIN32)
    # Prefix all shared libraries with 'lib'.
    set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
    # Prefix all static libraries with 'lib'.
    set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
find_package(TclStub 8.6 REQUIRED)  # TCL_INCLUDE_PATH TCL_STUB_LIBRARY TCL_TCLSH

message(STATUS "TCL_INCLUDE_PATH: ${TCL_INCLUDE_PATH}")
message(STATUS "TCL_STUB_LIBRARY: ${TCL_STUB_LIBRARY}")

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS  "-DTCL_THREADS -DVERSION=${PROJECT_VERSION} ${CMAKE_C_FLAGS}")
add_compile_definitions(BUILD_tjson)

enable_testing()
add_test(NAME AllUnitTests COMMAND ${TCL_TCLSH} ${CMAKE_CURRENT_SOURCE_DIR}/tests/all.tcl)

add_library(tjson src/library.c src/cJSON/cJSON.c src/jsonpath/jsonpath.c src/custom_triple_notation/custom_triple_notation.c)
set_target_properties(tjson PROPERTIES POSITION_INDEPENDENT_CODE ON)

include_directories(${TCL_INCLUDE_PATH})
if(BUILD_SHARED_LIBS)
    target_compile_definitions(tjson PRIVATE USE_TCL_STUBS)
    target_link_libraries(tjson PRIVATE ${TCL_STUB_LIBRARY})
else()
    # The application links Tcl and registers Tjson_Init with Tcl_StaticLibrary.
    target_compile_definitions(tjson PRIVATE STATIC_BUILD)
endif()

install(TARGETS ${TARGET}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
)

if(BUILD_SHARED_LIBS)
    configure_file(pkgIndex.tcl.in pkgIndex.tcl @ONLY)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
    )
endif()
