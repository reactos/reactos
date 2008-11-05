<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="timedate" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_TIMEDATE}" installbase="system32" installname="timedate.cpl" unicode="yes">
	<importlibrary definition="timedate.def" />
	<include base="timedate">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ws2_32</library>
	<library>iphlpapi</library>
	<file>clock.c</file>
	<file>dateandtime.c</file>
	<file>internettime.c</file>
	<file>monthcal.c</file>
	<file>ntpclient.c</file>
	<file>timedate.c</file>
	<file>timezone.c</file>
	<file>timedate.rc</file>
</module>
