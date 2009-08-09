<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgrist" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdgrist.dll">
	<importlibrary definition="kbdgrist.spec" />
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<file>kbdgrist.c</file>
	<file>kbdgrist.rc</file>
</module>
