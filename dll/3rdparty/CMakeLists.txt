
if(NSWPAT)
add_subdirectory(dxtn)
endif()
add_subdirectory(libjpeg)
add_subdirectory(libpng)
#add_subdirectory(libtiff)#FIXME : unresolved symbol _pei386_runtime_relocator
add_subdirectory(libxslt)
if(NOT MSVC)
add_subdirectory(mesa32)
endif()
