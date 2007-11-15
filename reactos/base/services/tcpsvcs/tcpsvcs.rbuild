<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="tcpsvcs" type="win32cui" installbase="system32" installname="tcpsvcs.exe">
	<include base="arp">.</include>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>iphlpapi</library>
	<library>ws2_32</library>
	<library>shlwapi</library>
	<library>advapi32</library>
	<library>user32</library>
	<family>services</family>
	<file>tcpsvcs.c</file>
	<file>skelserver.c</file>
	<file>echo.c</file>
	<file>discard.c</file>
	<file>daytime.c</file>
	<file>qotd.c</file>
	<file>chargen.c</file>
	<file>tcpsvcs.rc</file>
	<pch>tcpsvcs.h</pch>
</module>
