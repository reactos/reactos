<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="idndl" type="win32dll" installname="idndl.dll" entrypoint="0">
	<library>kernel32</library>
	<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
	<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	<redefine name="WINVER">0x600</redefine>
	<file>idndl.cpp</file>
	<importlibrary definition="idndl-$(ARCH).def" />
</module>
