<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="clb" type="win32dll" baseaddress="${BASEADDRESS_CLB}" installbase="system32" installname="clb.dll">
	<importlibrary definition="clb.def" />
	<include base="clb">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<file>clb.c</file>
	<file>clb.rc</file>
	<pch>precomp.h</pch>
</module>
