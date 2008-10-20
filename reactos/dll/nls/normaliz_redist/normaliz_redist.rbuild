<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="normaliz_redist_data" type="staticlibrary">
	<include base="icu4ros">icu/source/common</include>
	<directory name="data"><file>icudt38.c</file></directory>
</module>
<module name="normaliz_redist" type="win32dll" installname="normaliz_redist.dll">
	<library>normalize</library>
	<library>idna</library>
	<library>icu4ros</library>
	<library>normaliz_redist_data</library>
	<library>libcntpr</library>
	<library>kernel32</library>
	<compilerflag compiler="cpp">-fno-exceptions</compilerflag>
	<compilerflag compiler="cpp">-fno-rtti</compilerflag>
	<include base="icu4ros">icu/source/common</include>
	<importlibrary definition="normaliz.def" />
	<file>normaliz.cpp</file>
</module>
