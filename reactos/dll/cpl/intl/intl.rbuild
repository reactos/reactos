<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="intl" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_INTL}" installbase="system32" installname="intl.cpl" unicode="yes" crt="msvcrt">
	<importlibrary definition="intl.spec" />
	<include base="intl">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<library>setupapi</library>
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
