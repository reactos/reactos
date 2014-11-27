
#add_executable(pefixup pefixup.c)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(bin2c bin2c.c)
add_executable(gendib gendib/gendib.c)
add_executable(geninc geninc/geninc.c)
add_executable(mkshelllink mkshelllink/mkshelllink.c)
add_executable(obj2bin obj2bin/obj2bin.c)
add_executable(spec2def spec2def/spec2def.c)
add_executable(utf16le utf16le/utf16le.cpp)

add_subdirectory(cabman)
add_subdirectory(cdmake)
add_subdirectory(mkhive)
add_subdirectory(unicode)
add_subdirectory(widl)
add_subdirectory(wpp)

if(NOT MSVC)
    add_subdirectory(log2lines)
    add_subdirectory(rsym)
endif()
