
list(APPEND SOURCE evalcmd.cpp)

add_library(evalcmd STATIC ${SOURCE})
add_dependencies(evalcmd psdk)

if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    target_compile_definitions(evalcmd PRIVATE _SHELL32_)
else()
    target_compile_definitions(evalcmd PRIVATE _SHLWAPI_)
endif()
