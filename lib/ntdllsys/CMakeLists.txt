
get_directory_property(defines COMPILE_DEFINITIONS)

foreach(arg ${defines})
  set(result_defs "${result_defs} -D${arg}")
endforeach(arg ${defines})

set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -x assembler-with-cpp -o <OBJECT> ${result_defs} -D__ASM__ <FLAGS> -c <SOURCE>")

set_source_files_properties(${REACTOS_BINARY_DIR}/lib/ntdllsys/ntdll.S PROPERTIES GENERATED TRUE)

add_library(ntdllsys ${REACTOS_BINARY_DIR}/lib/ntdllsys/ntdll.S)
add_dependencies(ntdllsys ntdll_S)
