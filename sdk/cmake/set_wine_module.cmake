
function(set_wine_module TARGET)
    include_directories(BEFORE
        ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
        ${REACTOS_BINARY_DIR}/sdk/include/reactos/wine)
endfunction()
