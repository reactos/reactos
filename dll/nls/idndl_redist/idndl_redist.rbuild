<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="idndl_redist" type="win32dll" installname="idndl_redist.dll" crt="libcntpr">
	<library>kernel32</library>
	<library>scripts</library>
	<compilerflag compiler="cpp">-fno-exceptions</compilerflag>
	<compilerflag compiler="cpp">-fno-rtti</compilerflag>
	<linkerflag>--entry=0</linkerflag>
	<include base="icu4ros">icu/source/common</include>
	<include base="scripts">.</include>
	<importlibrary definition="idndl.def" />
	<file>idndl.cpp</file>
	<file>stubs.cpp</file>
</module>
