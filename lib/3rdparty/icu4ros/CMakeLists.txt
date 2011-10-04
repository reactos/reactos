
include_directories(
    icu/source/common
    icu/source/i18n)

add_definitions(
    -DHAVE_EXPAT_CONFIG_H
    -DU_STATIC_IMPLEMENTATION
    -DU_HAVE_INTTYPES_H
    -DUCONFIG_NO_FILE_IO=1
    -DICU_NO_USER_DATA_OVERRIDE=1)

list(APPEND SOURCE 
  stubs.cpp
  icu/source/common/bmpset.cpp
  icu/source/common/uhash_us.cpp
  icu/source/common/uidna.cpp
  icu/source/common/unifilt.cpp
  icu/source/common/unifunct.cpp
  icu/source/common/uniset.cpp
  icu/source/common/unisetspan.cpp
  icu/source/common/unistr.cpp
  icu/source/common/unorm.cpp
  icu/source/common/uobject.cpp
  icu/source/common/uobject.cpp
  icu/source/common/uset.cpp
  icu/source/common/usprep.cpp
  icu/source/common/uiter.cpp
  icu/source/common/util.cpp
  icu/source/common/cmemory.c
  icu/source/common/cstring.c
  icu/source/common/locmap.c
  icu/source/common/punycode.c
  icu/source/common/ubidi_props.c
  icu/source/common/ucln_cmn.c
  icu/source/common/ucmndata.c
  icu/source/common/ucol_swp.c
  icu/source/common/udata.c
  icu/source/common/udatamem.c
  icu/source/common/udataswp.c
  icu/source/common/uhash.c
  icu/source/common/uinit.c
  icu/source/common/uinvchar.c
  icu/source/common/umapfile.c
  icu/source/common/umath.c
  icu/source/common/umutex.c
  icu/source/common/ustring.c
  icu/source/common/ustrtrns.c
  icu/source/common/utf_impl.c
  icu/source/common/utrie.c)

add_library(icu4ros ${SOURCE})
