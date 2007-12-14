<module name="kbdbgt" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdbgt.dll" allowwarnings="true">
	<importlibrary definition="kbdbgt.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbgt.c</file>
	<file>kbdbgt.rc</file>
</module>