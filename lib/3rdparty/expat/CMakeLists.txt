
include_directories(
    lib
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/expat)

add_definitions(-DHAVE_EXPAT_CONFIG_H)

list(APPEND SOURCE
    lib/xmlparse.c
    lib/xmlrole.c
    lib/xmltok.c)

add_library(expat ${SOURCE})
