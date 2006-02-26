<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="kbdhu" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdhu.dll" allowwarnings="true">
	<importlibrary definition="kbdhu.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdhu.c</file>
	<file>kbdhu.rc</file>
</module>
</rbuild>
