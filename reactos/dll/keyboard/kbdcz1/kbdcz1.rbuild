<module name="kbdcz1" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdcz1.dll" allowwarnings="true">
	<importlibrary definition="kbdcz1.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdcz1.c</file>
	<file>kbdcz1.rc</file>
</module>
