<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="inetcpl" type="win32dll" extension=".dll" baseaddress="${BASEADDRESS_INETCPL}" installbase="system32" installname="inetcpl.cpl" unicode="yes">
	<importlibrary definition="inetcpl.spec" />
	<include base="inetcpl">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library delayimport="true">cryptui</library>
	<library delayimport="true">wininet</library>
	<library delayimport="true">ole32</library>
	<library delayimport="true">urlmon</library>		
	<library>delayimp</library>
	<library>wine</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>gdi32</library>
	<library>shlwapi</library>
	<file>inetcpl.c</file>
	<file>content.c</file>
	<file>general.c</file>
	<file>security.c</file>
	<file>inetcpl.rc</file>
</module>
