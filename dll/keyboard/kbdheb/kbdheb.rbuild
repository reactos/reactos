<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="kbdheb" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdheb.dll" allowwarnings="true">
	<importlibrary definition="kbdheb.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdheb.c</file>
	<file>kbdheb.rc</file>
</module>
</rbuild>
