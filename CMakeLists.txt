cmake_minimum_required(VERSION 3.22.1)
project(tjson VERSION 1.0.0 LANGUAGES C)

set(TARGET tjson)
set(CMAKE_C_STANDARD   11)
set(CMAKE_C_STANDARD_REQUIRED true)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS  "-DVERSION=${PROJECT_VERSION} ${CMAKE_C_FLAGS}")

add_library(tjson MODULE library.c)
set_target_properties(tjson PROPERTIES POSITION_INDEPENDENT_CODE ON)

include(${CMAKE_CURRENT_LIST_DIR}/FindTCL.cmake)

if (NAVISERVER)
    include_directories(${NAVISERVER}/include)
    target_link_directories(tjson PRIVATE ${NAVISERVER}/lib)
    target_link_libraries(tjson PRIVATE cjson ${TCL_LIBRARY})
    target_compile_options(tjson PRIVATE -DUSE_NAVISERVER)
    install(TARGETS ${TARGET}
            LIBRARY DESTINATION ${NAVISERVER}/bin
    )
else ()
    include_directories(cJSON ${TCL_INCLUDE_PATH})
    target_link_libraries(tjson PRIVATE cjson ${TCL_LIBRARY})
    get_filename_component(TCL_LIBRARY_PATH "${TCL_LIBRARY}" PATH)

    install(TARGETS ${TARGET}
            LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
    )

    configure_file(pkgIndex.tcl.in pkgIndex.tcl @ONLY)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${TARGET}${PROJECT_VERSION}
    )
endif()
