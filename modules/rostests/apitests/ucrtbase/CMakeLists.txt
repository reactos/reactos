
# Hack to replace the old CRT include directory with the UCRT include directory
get_property(INCLUDE_DIRS DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRS "${REACTOS_SOURCE_DIR}/sdk/include/crt")
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${INCLUDE_DIRS})
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/ucrt)

add_definitions(
    -DTEST_UCRTBASE
    -DUSE_WINE_TODOS
)

list(APPEND SOURCE
    ../crt/_finite.c
    ../crt/_isnan.c
    ../crt/acos.c
    ../crt/asin.c
    ../crt/atan.c
    ../crt/ceil.c
    ../crt/cos.c
    ../crt/exp.c
    ../crt/log.c
    ../crt/log10.c
    ../crt/round.c
    ../crt/sin.c
    ../crt/sqrt.c
    ../crt/tan.c
    testlist.c
)

add_executable(ucrtbase_apitest ${SOURCE})
set_module_type(ucrtbase_apitest win32cui)

target_link_libraries(ucrtbase_apitest ${PSEH_LIB} chkstk)
if(NOT MSVC)
    # Add this manually here, so it can link to ucrtbase.dll and vcstartup
    target_link_libraries(ucrtbase_apitest -lgcc)
endif()

add_importlibs(ucrtbase_apitest ucrtbase kernel32 ntdll)
add_rostests_file(TARGET ucrtbase_apitest)
