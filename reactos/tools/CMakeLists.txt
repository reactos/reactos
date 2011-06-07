
#add_executable(pefixup pefixup.c)

add_subdirectory(cabman)
add_subdirectory(cdmake)
add_subdirectory(gendib)
add_subdirectory(geninc)
add_subdirectory(mkhive)
add_subdirectory(obj2bin)
add_subdirectory(spec2def)
add_subdirectory(unicode)

if(NOT MSVC)
add_subdirectory(widl)
add_subdirectory(wpp)
add_subdirectory(wrc)
endif()
