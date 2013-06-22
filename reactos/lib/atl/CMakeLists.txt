
set_cpp(WITH_RTTI)

add_definitions(
    -DUNICODE -D_UNICODE
    -DROS_Headers)

add_library(atlnew atlbase.cpp atlcore.cpp)
add_dependencies(atlnew psdk)

