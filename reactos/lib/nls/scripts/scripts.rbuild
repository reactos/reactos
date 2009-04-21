<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="scripts" type="staticlibrary">
	<library>icu4ros</library>
	<redefine name="WINVER">0x600</redefine>
	<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
	<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	<include base="icu4ros">icu/source/common</include>
	<file>scripts.cpp</file>
</module>
