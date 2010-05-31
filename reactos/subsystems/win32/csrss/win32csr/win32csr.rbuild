<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="win32csr" type="win32dll" installbase="system32" installname="win32csr.dll">
	<importlibrary definition="win32csr.spec" />
	<include base="win32csr">.</include>
	<include base="csrss">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<include base="ReactOS">include/reactos/drivers</include>
	<include base="console">.</include>
	<compilerflag compilerset="gcc">-fms-extensions</compilerflag>
	<library>ntdll</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>win32ksys</library>
	<library>psapi</library>
	<library>pseh</library>
	<pch>w32csr.h</pch>
	<file>alias.c</file>
	<file>coninput.c</file>
	<file>conoutput.c</file>
	<file>console.c</file>
	<file>desktopbg.c</file>
	<file>dllmain.c</file>
	<file>exitros.c</file>
	<file>guiconsole.c</file>
	<file>handle.c</file>
	<file>harderror.c</file>
	<file>tuiconsole.c</file>
	<file>appswitch.c</file>
	<file>win32csr.rc</file>
</module>
