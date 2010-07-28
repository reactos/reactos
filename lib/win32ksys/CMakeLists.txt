
get_directory_property(defines COMPILE_DEFINITIONS)

foreach(arg ${defines})
  set(result_defs "${result_defs} -D${arg}")
endforeach(arg ${defines})

set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -x assembler-with-cpp -o <OBJECT> ${result_defs} -D__ASM__ <FLAGS> -c <SOURCE>")

set_source_files_properties(${REACTOS_BINARY_DIR}/lib/win32ksys/win32k.S PROPERTIES GENERATED TRUE)

add_library(win32ksys ${REACTOS_BINARY_DIR}/lib/win32ksys/win32k.S)
add_dependencies(win32ksys win32k_S)
