
list(APPEND SOURCE
    cicarray.cpp
    cicbase.cpp
    ciccaret.cpp
    cicfmap.cpp
    cicimc.cpp
    cicreg.cpp
    cicuif.cpp)

add_library(cicero_a STATIC ${SOURCE})
add_library(cicero_w STATIC ${SOURCE})

target_include_directories(cicero_a PRIVATE
    ${REACTOS_SOURCE_DIR}/sdk/include
    ${REACTOS_SOURCE_DIR}/sdk/include/ddk
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos)
target_include_directories(cicero_w PRIVATE
    ${REACTOS_SOURCE_DIR}/sdk/include
    ${REACTOS_SOURCE_DIR}/sdk/include/ddk
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos)

target_compile_definitions(cicero_w PRIVATE UNICODE _UNICODE)
