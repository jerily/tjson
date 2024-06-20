cmake_minimum_required(VERSION 3.22.1)
project(tjson VERSION 1.0.13 LANGUAGES C)

set(TARGET tjson)
set(CMAKE_C_STANDARD   11)
set(CMAKE_C_STANDARD_REQUIRED true)
set(THREADS_PREFER_PTHREAD_FLAG ON)

if(WIN32)
    # Prefix all shared libraries with 'lib'.
    set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
    # Prefix all static libraries with 'lib'.
    set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(TCL 8.6.13 REQUIRED)  # TCL_INCLUDE_PATH TCL_LIBRARY

message(STATUS "TCL_INCLUDE_PATH: ${TCL_INCLUDE_PATH}")
message(STATUS "TCL_LIBRARY: ${TCL_LIBRARY}")

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS  "-DTCL_THREADS -DVERSION=${PROJECT_VERSION} ${CMAKE_C_FLAGS}")

enable_testing()
add_test(NAME AllUnitTests COMMAND tclsh8.6 ${CMAKE_CURRENT_SOURCE_DIR}/tests/all.tcl)

add_library(tjson SHARED src/library.c src/cJSON/cJSON.c src/jsonpath/jsonpath.c src/custom_triple_notation/custom_triple_notation.c)
set_target_properties(tjson PROPERTIES POSITION_INDEPENDENT_CODE ON)

include_directories(${TCL_INCLUDE_PATH})
target_link_libraries(tjson PRIVATE ${TCL_LIBRARY})
get_filename_component(TCL_LIBRARY_PATH "${TCL_LIBRARY}" PATH)

install(TARGETS ${TARGET}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
)

configure_file(pkgIndex.tcl.in pkgIndex.tcl @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
)
