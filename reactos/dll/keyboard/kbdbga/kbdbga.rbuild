<module name="kbdbga" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdbga.dll" allowwarnings="true">
	<importlibrary definition="kbdbga.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbga.c</file>
	<file>kbdbga.rc</file>
</module>