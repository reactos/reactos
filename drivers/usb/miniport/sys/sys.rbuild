<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="sys_base" type="objectlibrary">
	<define name="__USE_W32API" />
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<file>ros_wrapper.c</file>
	<file>linuxwrapper.c</file>
</module>
</rbuild>
