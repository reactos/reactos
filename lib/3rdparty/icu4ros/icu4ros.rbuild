<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="icu4ros" type="staticlibrary">
	<define name="U_STATIC_IMPLEMENTATION" />
	<define name="U_HAVE_INTTYPES_H" />
	<define name="UCONFIG_NO_FILE_IO">1</define>
	<define name="ICU_NO_USER_DATA_OVERRIDE">1</define>
	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>
	<include base="icu4ros">icu/source/common</include>
	<include base="icu4ros">icu/source/i18n</include>
	<file>stubs.cpp</file>
	<directory name="icu">
	<directory name="source">
	<directory name="common">
		<file>bmpset.cpp</file>
		<file>uhash_us.cpp</file>
		<file>uidna.cpp</file>
		<file>uiter.cpp</file>
		<file>unifilt.cpp</file>
		<file>unifunct.cpp</file>
		<file>uniset.cpp</file>
		<file>unisetspan.cpp</file>
		<file>unistr.cpp</file>
		<file>unorm.cpp</file>
		<file>uobject.cpp</file>
		<file>uset.cpp</file>
		<file>usprep.cpp</file>
		<file>util.cpp</file>
		<file>uvector.cpp</file>
		<file>cmemory.c</file>
		<file>cstring.c</file>
		<file>locmap.c</file>
		<file>punycode.c</file>
		<file>ucln_cmn.c</file>
		<file>ucmndata.c</file>
		<file>ucol_swp.c</file>
		<file>ubidi_props.c</file>
		<file>udata.c</file>
		<file>udatamem.c</file>
		<file>udataswp.c</file>
		<file>uhash.c</file>
		<file>uinit.c</file>
		<file>uinvchar.c</file>
		<file>umapfile.c</file>
		<file>umath.c</file>
		<file>umutex.c</file>
		<file>ustring.c</file>
		<file>ustrtrns.c</file>
		<file>utf_impl.c</file>
		<file>utrie.c</file>
	</directory>
	</directory>
	</directory>
</module>
