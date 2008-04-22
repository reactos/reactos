<module name="kbdlt" type="kernelmodedll" entrypoint="0" installbase="system32" installname="kbdlt.dll" allowwarnings="true">
	<importlibrary definition="kbdlt.def" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0500</define>
	<file>kbdlt.c</file>
	<file>kbdlt.rc</file>
</module>
