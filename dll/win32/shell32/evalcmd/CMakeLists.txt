
add_library(evalcmd STATIC evalcmd.cpp)
add_dependencies(evalcmd psdk)
target_link_libraries(evalcmd PRIVATE pathcch_static)
target_compile_definitions(evalcmd PRIVATE STATIC_PATHCCH)

if(DLL_EXPORT_VERSION GREATER_EQUAL 0x600)
    target_compile_definitions(evalcmd PRIVATE _SHELL32_)
else()
    target_compile_definitions(evalcmd PRIVATE _SHLWAPI_)
endif()
