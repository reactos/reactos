<module name="kbdfr" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdfr.dll" allowwarnings="true">
	<importlibrary definition="kbdfr.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdfr.c</file>
	<file>kbdfr.rc</file>
</module>
