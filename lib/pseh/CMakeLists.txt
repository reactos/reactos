
file(GLOB_RECURSE ARCH_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*.S)

get_directory_property(defines COMPILE_DEFINITIONS)

foreach(arg ${defines})
  set(result_defs "${result_defs} -D${arg}")
endforeach(arg ${defines})

set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -x assembler-with-cpp -o <OBJECT> ${result_defs} -D__ASM__ <FLAGS> -c <SOURCE>")

file(GLOB SOURCE "*.c")

add_library(pseh ${ARCH_SOURCE} ${SOURCE})
add_dependencies(pseh psdk)

