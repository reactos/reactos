<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="idndl" type="win32dll" installname="idndl.dll">
	<library>kernel32</library>
	<compilerflag>-fno-exceptions</compilerflag>
	<compilerflag>-fno-rtti</compilerflag>
	<linkerflag>-Wl,--entry,0</linkerflag>
	<file>idndl.cpp</file>
	<importlibrary definition="idndl.def" />
</module>
