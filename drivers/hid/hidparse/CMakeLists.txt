
spec2def(hidparse.sys hidparse.spec ADD_IMPORTLIB)
add_definitions(-DDEBUG_MODE)

include_directories(
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/hidparser)

list(APPEND SOURCE
    hidp_km.c
    parser.c
    parser.h
    hidparse.c
    hidparse.h)

add_library(hidparse MODULE ${SOURCE} hidparse.rc ${CMAKE_CURRENT_BINARY_DIR}/hidparse.def)
set_module_type(hidparse kernelmodedriver)
add_importlibs(hidparse ntoskrnl)
target_link_libraries(hidparse hidparser_km)
add_cd_file(TARGET hidparse DESTINATION reactos/system32/drivers NO_CAB FOR all)
