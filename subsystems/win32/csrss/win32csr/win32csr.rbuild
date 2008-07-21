<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="win32csr" type="win32dll" installbase="system32" installname="win32csr.dll">
	<importlibrary definition="win32csr.def" />
	<include base="win32csr">.</include>
	<include base="csrss">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<include base="ReactOS">include/reactos/drivers</include>
	<include base="console">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>psapi</library>
	<library>msvcrt</library>
	<pch>w32csr.h</pch>
	<file>conio.c</file>
	<file>desktopbg.c</file>
	<file>dllmain.c</file>
	<file>exitros.c</file>
	<file>guiconsole.c</file>
	<file>tuiconsole.c</file>
	<file>win32csr.rc</file>
</module>
