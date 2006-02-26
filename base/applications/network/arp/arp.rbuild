<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="arp" type="win32cui" installbase="system32" installname="arp.exe">
	<include base="arp">.</include>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>iphlpapi</library>
	<library>ws2_32</library>
	<library>shlwapi</library>
	<file>arp.c</file>
	<file>arp.rc</file>
</module>

</rbuild>
