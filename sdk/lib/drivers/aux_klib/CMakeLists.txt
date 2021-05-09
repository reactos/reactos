
add_library(aux_klib aux_klib.c)
target_link_libraries(aux_klib ${PSEH_LIB})
add_importlibs(aux_klib ntoskrnl)
