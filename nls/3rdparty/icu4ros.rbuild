<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="icu4ros" type="staticlibrary" allowwarnings="true">
	<define name="U_STATIC_IMPLEMENTATION" />
	<define name="U_HAVE_INTTYPES_H" />
	<compilerflag>-fno-exceptions</compilerflag>
	<compilerflag>-fno-rtti</compilerflag>
	<include base="icu4ros">icu/source/common</include>
	<directory name="icu">
	<directory name="source">
	<directory name="common">
		<file>bmpset.cpp</file>
		<file>uhash_us.cpp</file>
		<file>uiter.cpp</file>
		<file>unifilt.cpp</file>
		<file>unifunct.cpp</file>
		<file>uniset.cpp</file>
		<file>unisetspan.cpp</file>
		<file>unistr.cpp</file>
		<file>unorm.cpp</file>
		<file>uobject.cpp</file>
		<file>uset.cpp</file>
		<file>util.cpp</file>
		<file>uvector.cpp</file>
		<file>cmemory.c</file>
		<file>cstring.c</file>
		<file>ucln_cmn.c</file>
		<file>udata.c</file>
		<file>udataswp.c</file>
		<file>uhash.c</file>
		<file>uinvchar.c</file>
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
