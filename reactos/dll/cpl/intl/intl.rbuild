<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="intl" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_INTL}" installbase="system32" installname="intl.cpl" unicode="yes">
	<importlibrary definition="intl.def" />
	<include base="intl">.</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<define name="WINVER">0x609</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<library>setupapi</library>
	<library>msvcrt</library>
	<library>shell32</library>
	<file>currency.c</file>
	<file>date.c</file>
	<file>generalp.c</file>
	<file>intl.c</file>
	<file>inplocale.c</file>
	<file>numbers.c</file>
	<file>time.c</file>
	<file>misc.c</file>
	<file>languages.c</file>
	<file>advanced.c</file>
	<file>sort.c</file>
	<file>intl.rc</file>
</module>
