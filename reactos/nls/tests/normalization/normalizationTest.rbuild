<?xml version="1.0"?>
<module name="normalizationTest" type="win32cui">
	<define name="U_HAVE_INTTYPES_H" />
	<include base="icu4ros">icu/source/common</include>
	<library>kernel32</library>
	<library>normaliz</library>
	<file>normalizationTest.c</file>
	<file>normalizationTest.tab.c</file>
	<file>normalizationTest.yy.c</file>
</module>
