<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="normaliz_redist" type="win32dll" installname="normaliz.dll">
	<library>normalize</library>
	<library>icu4ros</library>
	<compilerflag>-fno-exceptions</compilerflag>
	<compilerflag>-fno-rtti</compilerflag>
	<importlibrary definition="normaliz.def" />
	<file>normaliz.cpp</file>
</module>
