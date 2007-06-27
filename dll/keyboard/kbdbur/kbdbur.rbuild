<module name="kbdbur" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdbur.dll" allowwarnings="true">
	<importlibrary definition="kbdbur.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdbur.c</file>
	<file>kbdbur.rc</file>
</module>
