cmake_minimum_required(VERSION 3.22.1)
project(tjson VERSION 1.0.6 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(TARGET tjson)
set(CMAKE_C_STANDARD   11)
set(CMAKE_C_STANDARD_REQUIRED true)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(TCL EXACT 8.6.13 REQUIRED)  # TCL_INCLUDE_PATH TCL_LIBRARY

message(STATUS "TCL_INCLUDE_PATH: ${TCL_INCLUDE_PATH}")
message(STATUS "TCL_LIBRARY: ${TCL_LIBRARY}")

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS  "-DTCL_THREADS -DVERSION=${PROJECT_VERSION} ${CMAKE_C_FLAGS}")

enable_testing()
add_test(NAME AllUnitTests COMMAND tclsh ${CMAKE_CURRENT_SOURCE_DIR}/tests/all.tcl)

add_library(tjson MODULE src/library.c src/cJSON/cJSON.c src/jsonpath/jsonpath.c src/custom_triple_notation/custom_triple_notation.c)
set_target_properties(tjson PROPERTIES POSITION_INDEPENDENT_CODE ON)

include_directories(${TCL_INCLUDE_PATH})
target_link_libraries(tjson PRIVATE ${TCL_LIBRARY})
get_filename_component(TCL_LIBRARY_PATH "${TCL_LIBRARY}" PATH)

install(TARGETS ${TARGET}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
)

configure_file(pkgIndex.tcl.in pkgIndex.tcl @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
)
