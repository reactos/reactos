
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
add_dependencies(cicero psdk)

add_library(cicero_include INTERFACE)
target_include_directories(cicero_include INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}>)
