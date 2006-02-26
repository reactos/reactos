<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="usbminiportcommon" type="objectlibrary">
	<define name="__USE_W32API" />
	<include>../linux</include>
	<include base="usbport">.</include>
	<file>cleanup.c</file>
	<file>close.c</file>
	<file>create.c</file>
	<file>fdo.c</file>
	<file>main.c</file>
	<file>misc.c</file>
	<file>pdo.c</file>
</module>
</rbuild>
