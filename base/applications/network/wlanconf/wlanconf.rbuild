<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="wlanconf" type="win32cui" installbase="system32" installname="wlanconf.exe">
	<include base="wlanconf">.</include>
	<include base="ReactOS">include/reactos/drivers/ndisuio</include>
	<library>iphlpapi</library>
	<file>wlanconf.c</file>
	<file>wlanconf.rc</file>
</module>
