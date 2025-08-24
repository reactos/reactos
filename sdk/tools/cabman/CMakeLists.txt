
#add_definitions(-DDBG)

list(APPEND SOURCE
    cabinet.cxx
    cabinet.h
    dfp.cxx
    dfp.h
    cabman.cxx
    cabman.h
    mszip.cxx
    mszip.h
    raw.cxx
    raw.h
    CCFDATAStorage.cxx
    CCFDATAStorage.h)

add_host_tool(cabman ${SOURCE})
target_link_libraries(cabman PRIVATE host_includes zlibhost)
set_property(TARGET cabman PROPERTY CXX_STANDARD 11)

if(MSVC AND ARCH STREQUAL "amd64")
    # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
    target_compile_options(cabman PRIVATE "/wd4267")
endif()
