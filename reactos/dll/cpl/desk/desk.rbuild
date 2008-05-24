<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="desk" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_DESK}" installbase="system32" installname="desk.cpl" unicode="true">
	<importlibrary definition="desk.def" />
	<include base="desk">.</include>
	<include base="desk" root="intermediate">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_WIN32" />
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>ole32</library>
	<library>setupapi</library>
	<library>shell32</library>
	<library>ntdll</library>
	<library>msimg32</library>
	<library>msvcrt</library>
	<library>uuid</library>
	<file>advmon.c</file>
	<file>appearance.c</file>
	<file>background.c</file>
	<file>classinst.c</file>
	<file>desk.c</file>
	<file>devsett.c</file>
	<file>dibitmap.c</file>
	<file>misc.c</file>
	<file>preview.c</file>
	<file>screensaver.c</file>
	<file>advappdlg.c</file>
	<file>settings.c</file>
	<file>monslctl.c</file>
	<file>desk.rc</file>

	<!-- Auto generated stuff -->
	<autoresource>auto.rc</autoresource>

	<!-- Authors -->
	<developer>ekohl</developer>

	<metadata description="ReactOS Display ControlPanel Applet" />

	<directory name="lang">
		<xi:include href="lang/lang.rbuild" />
	</directory>
</module>
