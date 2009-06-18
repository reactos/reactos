<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="idndl_redist" type="win32dll" installname="idndl_redist.dll" crt="libcntpr">
	<library>kernel32</library>
	<library>scripts</library>
	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>
	<linkerflag linkerset="ld">--entry=0</linkerflag>
	<include base="icu4ros">icu/source/common</include>
	<include base="scripts">.</include>
	<importlibrary definition="idndl.def" />
	<file>idndl.cpp</file>
	<file>stubs.cpp</file>
</module>
