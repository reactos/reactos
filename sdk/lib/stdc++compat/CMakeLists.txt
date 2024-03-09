
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)

list(APPEND SOURCE
    stdc++compat.cpp)

add_library(stdc++compat ${SOURCE} ${cpprt_asm})
set_target_cpp_properties(stdc++compat WITH_EXCEPTIONS)
add_dependencies(stdc++compat xdk)
target_link_libraries(stdc++compat libmsvcrt)
