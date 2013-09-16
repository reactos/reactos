add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/l_intl.nls DESTINATION reactos/system32 NO_CAB FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/c_437.nls DESTINATION reactos/system32 NO_CAB FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/c_1252.nls DESTINATION reactos/system32 NO_CAB FOR all)

list(APPEND NLS_FILES
    c_037.nls
    c_424.nls
    c_500.nls
    c_737.nls
    c_775.nls
    c_850.nls
    c_852.nls
    c_855.nls
    c_856.nls
    c_857.nls
    c_860.nls
    c_861.nls
    c_862.nls
    c_863.nls
    c_864.nls
    c_865.nls
    c_866.nls
    c_869.nls
    c_874.nls
    c_875.nls
    c_878.nls
    c_932.nls
    c_936.nls
    c_949.nls
    c_950.nls
    c_1006.nls
    c_1026.nls
    c_1250.nls
    c_1251.nls
    c_1253.nls
    c_1254.nls
    c_1255.nls
    c_1256.nls
    c_1257.nls
    c_1258.nls
    c_10000.nls
    c_10006.nls
    c_10007.nls
    c_10029.nls
    c_10079.nls
    c_10081.nls
    c_20127.nls
    c_20866.nls
    c_21866.nls
    c_28591.nls
    c_28592.nls
    c_28593.nls
    c_28594.nls
    c_28595.nls
    c_28596.nls
    c_28597.nls
    c_28598.nls
    c_28599.nls
    c_28600.nls
    c_28603.nls
    c_28604.nls
    c_28605.nls
    c_28606.nls
    unicode.nls
    geo.nls
    sorttbls.nls
    locale.nls
    sortkey.nls
    ctype.nls
    l_except.nls
)
    
    
foreach(item ${NLS_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/system32 FOR all)
endforeach(item)
