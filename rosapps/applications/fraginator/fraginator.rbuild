<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../tools/rbuild/project.dtd">
<group>
	<module name="frag" type="win32gui" installbase="system32" installname="frag.exe" unicode="yes">
		<include base="frag">.</include>
		<define name="WINVER">0x0501</define>
		<define name="_WIN32_IE">0x0600</define>
		<library>kernel32</library>
		<library>advapi32</library>
		<library>ntdll</library>
		<library>comctl32</library>
		<library>msvcrt</library>
		<file>Fraginator.cpp</file>
		<file>MainDialog.cpp</file>
		<file>Defragment.cpp</file>
		<file>DriveVolume.cpp</file>
		<file>ReportDialog.cpp</file>
		<file>Unfrag.cpp</file>
	</module>

	<module name="unfrag" type="win32cui" installbase="system32" installname="unfrag.exe" unicode="yes">
		<include base="unfrag">.</include>
		<define name="WINVER">0x0501</define>
		<define name="_WIN32_IE">0x0600</define>
		<library>kernel32</library>
		<library>advapi32</library>
		<library>ntdll</library>
		<file>Unfrag.cpp</file>
		<file>Defragment.cpp</file>
		<file>DriveVolume.cpp</file>
	</module>
</group>
