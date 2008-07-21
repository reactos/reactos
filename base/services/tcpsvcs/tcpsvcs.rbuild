<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="tcpsvcs" type="win32cui" installbase="system32" installname="tcpsvcs.exe" unicode="yes">
	<include base="arp">.</include>
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>advapi32</library>
	<file>tcpsvcs.c</file>
	<file>skelserver.c</file>
	<file>echo.c</file>
	<file>discard.c</file>
	<file>daytime.c</file>
	<file>qotd.c</file>
	<file>chargen.c</file>
	<file>tcpsvcs.rc</file>
	<file>log.c</file>
	<pch>tcpsvcs.h</pch>
</module>
