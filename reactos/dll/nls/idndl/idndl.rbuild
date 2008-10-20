<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="idndl" type="win32dll" installname="idndl.dll">
	<library>kernel32</library>
	<compilerflag compiler="cpp">-fno-exceptions</compilerflag>
	<compilerflag compiler="cpp">-fno-rtti</compilerflag>
	<entrypoint>0</entrypoint>
	<file>idndl.cpp</file>
	<importlibrary definition="idndl.def" />
</module>
