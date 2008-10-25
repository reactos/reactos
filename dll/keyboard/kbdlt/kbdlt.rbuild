<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdlt" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdlt.dll" allowwarnings="true">
	<importlibrary definition="kbdlt.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdlt.c</file>
	<file>kbdlt.rc</file>
</module>
