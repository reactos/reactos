
list(APPEND SOURCE
    cicarray.cpp
    cicbase.cpp
    ciccaret.cpp
    cicfmap.cpp
    cicimc.cpp
    cicreg.cpp
    cicuif.cpp)

add_library(cicero STATIC ${SOURCE})
target_compile_definitions(cicero PRIVATE UNICODE _UNICODE)
target_link_libraries(cicero PRIVATE cppstl)
add_dependencies(cicero psdk reactos_idls cppstl)
target_include_directories(cicero INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}>)
