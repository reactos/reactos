<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvidctl" type="win32dll" baseaddress="${BASEADDRESS_MSVIDCTL}" installbase="system32" installname="msvidctl.dll">
	<importlibrary definition="msvidctl.spec" />
	<include base="msvidctl">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>msvcrt</library>
	<library>strmiids</library>
	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>
	<group compilerset="msc">
		<compilerflag compiler="cxx">/GR-</compilerflag>
	</group>

	<file>classfactory.cpp</file>
	<file>enumtuningspaces.cpp</file>
	<file>msvidctl.cpp</file>
	<file>msvidctl.rc</file>
	<file>tunerequest.cpp</file>
	<file>tuningspace.cpp</file>
	<file>tuningspace_container.cpp</file>
</module>
</group>
