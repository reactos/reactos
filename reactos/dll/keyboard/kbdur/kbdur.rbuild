<module name="kbdur" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdur.dll" allowwarnings="true">
	<importlibrary definition="kbdur.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdur.c</file>
	<file>kbdur.rc</file>
</module>
