<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdlt1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdlt1.dll" allowwarnings="true">
	<importlibrary definition="kbdlt1.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdlt1.c</file>
	<file>kbdlt1.rc</file>
</module>
