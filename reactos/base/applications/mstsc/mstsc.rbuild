<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="mstsc" type="win32gui" installbase="system32" installname="mstsc.exe" unicode="yes" allowwarnings="true">
	<include base="mstsc">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ws2_32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>ole32</library>
	<compilationunit name="unit.c">
		<file>bitmap.c</file>
		<file>bsops.c</file>
		<file>cache.c</file>
		<file>channels.c</file>
		<file>connectdialog.c</file>
		<file>iso.c</file>
		<file>licence.c</file>
		<file>mcs.c</file>
		<file>mppc.c</file>
		<file>orders.c</file>
		<file>pstcache.c</file>
		<file>rdp5.c</file>
		<file>rdp.c</file>
		<file>secure.c</file>
		<file>settings.c</file>
		<file>ssl_calls.c</file>
		<file>tcp.c</file>
		<file>uimain.c</file>
		<file>win32.c</file>
	</compilationunit>
	<file>rdc.rc</file>
	<pch>precomp.h</pch>
</module>
