
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

add_library(lsalib lsa.c)
add_dependencies(lsalib xdk)

add_library(ntlsalib lsa.c)
add_target_compile_definitions(ntlsalib _NTSYSTEM_)
add_dependencies(ntlsalib xdk)
