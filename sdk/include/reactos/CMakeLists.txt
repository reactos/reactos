
add_subdirectory(mc)

add_definitions(-D_MIDL_USE_GUIDDEF_)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND SOURCE
    msctf_undoc.idl)

add_idl_headers(reactos_idls ${SOURCE})
add_dependencies(reactos_idls psdk)
