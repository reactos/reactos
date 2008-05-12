<module name="kbdbgm" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdbgm.dll" allowwarnings="true">
	<importlibrary definition="kbdbgm.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbgm.c</file>
	<file>kbdbgm.rc</file>
</module>
