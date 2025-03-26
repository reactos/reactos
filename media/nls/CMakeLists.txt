
# These files are taken from here
# https://download.microsoft.com/download/C/F/7/CF713A5E-9FBC-4FD6-9246-275F65C0E498/Windows Supported Code Page Data Files.zip
# Except for c_1006.txt c_28600.txt c_28604.txt c_28606.txt c_424.txt which were manually created
list(APPEND GEN_NLS
    c_037
    c_10000
    c_10001
    c_10002
    c_10003
    c_10004
    c_10005
    c_10006
    c_10007
    c_10008
    c_10010
    c_10017
    c_10021
    c_10029
    c_10079
    c_10081
    c_10082
    c_1006
    c_1026
    c_1047
    c_1140
    c_1141
    c_1142
    c_1143
    c_1144
    c_1145
    c_1146
    c_1147
    c_1148
    c_1149
    c_1250
    c_1251
    c_1252
    c_1253
    c_1254
    c_1255
    c_1256
    c_1257
    c_1258
    c_1361
    c_20000
    c_20001
    c_20002
    c_20003
    c_20004
    c_20005
    c_20105
    c_20106
    c_20107
    c_20108
    c_20127
    c_20261
    c_20269
    c_20273
    c_20277
    c_20278
    c_20280
    c_20284
    c_20285
    c_20290
    c_20297
    c_20420
    c_20423
    c_20424
    c_20833
    c_20838
    c_20866
    c_20871
    c_20880
    c_20905
    c_20924
    c_21025
    c_21027
    c_21866
    c_28591
    c_28592
    c_28593
    c_28594
    c_28595
    c_28596
    c_28597
    c_28598
    c_28599
    c_28600
    c_28603
    c_28604
    c_28605
    c_28606
    c_29001
    c_424
    c_437
    c_500
    c_708
    c_720
    c_737
    c_775
    c_850
    c_852
    c_855
    c_857
    c_858
    c_860
    c_861
    c_862
    c_863
    c_864
    c_865
    c_866
    c_869
    c_870
    c_874
    c_875
    c_932
    c_936
    c_949
    c_950)

foreach(NLS ${GEN_NLS})
    add_custom_command(
        OUTPUT ${NLS}.nls
        COMMAND native-txt2nls ${CMAKE_CURRENT_SOURCE_DIR}/src/${NLS}.txt ${NLS}.nls
        DEPENDS native-txt2nls ${CMAKE_CURRENT_SOURCE_DIR}/src/${NLS}.txt)

    # Special case for system ones
    if ((NLS STREQUAL "c_437") OR (NLS STREQUAL "c_1252"))
        add_cd_file(TARGET gen_nls FILE ${CMAKE_CURRENT_BINARY_DIR}/${NLS}.nls DESTINATION reactos/system32 NO_CAB FOR all)
    else()
        add_cd_file(TARGET gen_nls FILE ${CMAKE_CURRENT_BINARY_DIR}/${NLS}.nls DESTINATION reactos/system32 FOR all)
    endif()
endforeach()

list(TRANSFORM GEN_NLS APPEND ".nls")
list(TRANSFORM GEN_NLS PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")
add_custom_target(gen_nls DEPENDS ${GEN_NLS})

# those ones were manually generated
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/l_intl.nls DESTINATION reactos/system32 NO_CAB FOR all)
list(APPEND NLS_FILES
    c_856.nls
    c_878.nls
    ctype.nls
    geo.nls
    l_except.nls
    locale.nls
    sortkey.nls
    sorttbls.nls
    unicode.nls)

foreach(item ${NLS_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/system32 FOR all)
endforeach(item)
