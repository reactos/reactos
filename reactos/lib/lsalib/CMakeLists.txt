
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)
add_library(lsalib lsa.c)
add_dependencies(lsalib xdk)
