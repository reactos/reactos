
# FIXME: CORE-5743

function(set_wine_module TARGET)
    include_directories(BEFORE
        ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
        ${REACTOS_BINARY_DIR}/sdk/include/reactos/wine)
endfunction()

# FIXME: Replace this call with set_wine_module
function(set_wine_module_FIXME TARGET)
    include_directories(BEFORE
        ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
        ${REACTOS_BINARY_DIR}/sdk/include/reactos/wine)
    target_compile_definitions(${TARGET} PRIVATE __WINESRC__)
endfunction()
