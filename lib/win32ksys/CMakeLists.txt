
set_source_files_properties(${REACTOS_BINARY_DIR}/lib/win32ksys/win32k.S PROPERTIES GENERATED TRUE)
add_library(win32ksys ${REACTOS_BINARY_DIR}/lib/win32ksys/win32k.S)
add_dependencies(win32ksys win32k_S asm)
