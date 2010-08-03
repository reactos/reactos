
set_source_files_properties(${REACTOS_BINARY_DIR}/lib/ntdllsys/ntdll.S PROPERTIES GENERATED TRUE)
add_library(ntdllsys ${REACTOS_BINARY_DIR}/lib/ntdllsys/ntdll.S)
add_dependencies(ntdllsys ntdll_S)
