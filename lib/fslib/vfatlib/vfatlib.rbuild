<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="vfatlib" type="staticlibrary">
	<include base="vfatlib">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<file>fat12.c</file>
	<file>fat16.c</file>
	<file>fat32.c</file>
	<file>vfatlib.c</file>
</module>
</rbuild>
