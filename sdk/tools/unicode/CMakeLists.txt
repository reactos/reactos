
add_definitions(-DDECLSPEC_HIDDEN=)

list(APPEND SOURCE
    c_037.c
    c_10000.c
    c_10001.c
    c_10002.c
    c_10003.c
    c_10004.c
    c_10005.c
    c_10006.c
    c_10007.c
    c_10008.c
    c_10010.c
    c_10017.c
    c_10021.c
    c_10029.c
    c_1006.c
    c_10079.c
    c_10081.c
    c_10082.c
    c_1026.c
    c_1250.c
    c_1251.c
    c_1252.c
    c_1253.c
    c_1254.c
    c_1255.c
    c_1256.c
    c_1257.c
    c_1258.c
    c_1361.c
    c_20127.c
    c_20866.c
    c_20932.c
    c_21866.c
    c_28591.c
    c_28592.c
    c_28593.c
    c_28594.c
    c_28595.c
    c_28596.c
    c_28597.c
    c_28598.c
    c_28599.c
    c_28600.c
    c_28603.c
    c_28604.c
    c_28605.c
    c_28606.c
    c_424.c
    c_437.c
    c_500.c
    c_737.c
    c_775.c
    c_850.c
    c_852.c
    c_855.c
    c_856.c
    c_857.c
    c_860.c
    c_861.c
    c_862.c
    c_863.c
    c_864.c
    c_865.c
    c_866.c
    c_869.c
    c_874.c
    c_875.c
    c_878.c
    c_932.c
    c_936.c
    c_949.c
    c_950.c
    casemap.c
    compose.c
    cptable.c
    mbtowc.c
    string.c
    utf8.c
    wctomb.c
    wctype.c)

add_library(unicode ${SOURCE})

if(MSVC)
    # Disable warning "'<': signed/unsigned mismatch"
    add_target_compile_flags(unicode "/wd4018")

    # Disable warning "unary minus operator applied to unsigned type, result still unsigned"
    add_target_compile_flags(unicode "/wd4146")

    # Disable warning "conversion from 'const WCHAR' to 'char', possible loss of data"
    add_target_compile_flags(unicode "/wd4244")
endif()
