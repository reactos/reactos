<module name="kbdno" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdno.dll" allowwarnings="true">
	<importlibrary definition="kbdno.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdno.c</file>
	<file>kbdno.rc</file>
</module>
