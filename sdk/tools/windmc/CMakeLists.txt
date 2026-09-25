
BISON_TARGET(mcparse mcparse.y ${CMAKE_CURRENT_BINARY_DIR}/mcparse.c
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/mcparse.h)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${REACTOS_SOURCE_DIR}/sdk/tools/port)

list(APPEND SOURCE
    windmc.c
    mclex.c
    winduni.c
    windmc_ros.c
    ${BISON_mcparse_OUTPUTS}
    ../port/getopt.c
    ../port/getopt1.c)

add_host_tool(windmc ${SOURCE})
target_compile_definitions(windmc PRIVATE WINDMC_VERSION="2.46")

# winduni.c uses the native Windows conversion API on Windows hosts and
# iconv elsewhere; without iconv it falls back to Latin-1 passthrough.
if(NOT CMAKE_HOST_WIN32)
    find_package(Iconv QUIET)
    if(Iconv_FOUND)
        target_compile_definitions(windmc PRIVATE HAVE_ICONV=1 ICONV_CONST=)
        target_link_libraries(windmc PRIVATE Iconv::Iconv)
    else()
        message(WARNING "iconv not found: windmc will only support ASCII codepage conversions")
    endif()
endif()

if(MSVC)
    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    target_compile_options(windmc PRIVATE "/wd4244")
endif()
