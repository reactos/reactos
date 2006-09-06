<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="telnet" type="win32cui" installbase="system32" installname="telnet.exe" allowwarnings ="true" stdlib="host">
	<include base="telnet">.</include>
	<define name="__USE_W32API" />
	<define name="__REACTOS__" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>user32</library>
	<directory name="src">
		<file>ansiprsr.cpp</file>
		<file>keytrans.cpp</file>
		<file>tcharmap.cpp</file>
		<file>tconsole.cpp</file>
		<file>tkeydef.cpp</file>
		<file>tkeymap.cpp</file>
		<file>tmapldr.cpp</file>
		<file>tmouse.cpp</file>
		<file>tnclass.cpp</file>
		<file>tnclip.cpp</file>
		<file>tncon.cpp</file>
		<file>tnconfig.cpp</file>
		<file>tnerror.cpp</file>
		<file>tnetwork.cpp</file>
		<file>tnmain.cpp</file>
		<file>tnmisc.cpp</file>
		<file>tscript.cpp</file>
		<file>tscroll.cpp</file>
		<file>ttelhndl.cpp</file>
	</directory>
	<file>telnet.rc</file>
</module>
