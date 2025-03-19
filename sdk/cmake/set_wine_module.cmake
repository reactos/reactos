
# FIXME: CORE-5743

function(set_wine_module TARGET)
    include_directories(BEFORE
        ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
        ${REACTOS_BINARY_DIR}/sdk/include/reactos/wine)
endfunction()

# FIXME: Replace this call with set_wine_module
function(set_wine_module_FIXME_1 TARGET)
    target_compile_definitions(${TARGET} PRIVATE -D__WINESRC__)
endfunction()

# FIXME: Replace this call with set_wine_module
function(set_wine_module_FIXME_2 TARGET)
    include_directories(BEFORE
        ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
        ${REACTOS_BINARY_DIR}/sdk/include/reactos/wine)
    target_compile_definitions(${TARGET} PRIVATE -D__WINESRC__)
endfunction()
