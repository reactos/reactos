<module name="kbdes" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdes.dll" allowwarnings="true">
	<importlibrary definition="kbdes.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdes.c</file>
	<file>kbdes.rc</file>
</module>
