add_definitions(
    -D__WINESRC__)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/include/psdk
    "/include/")

add_subdirectory(libs)
