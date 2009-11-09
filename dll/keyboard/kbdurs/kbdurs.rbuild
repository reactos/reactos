<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdurs" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdurs.dll">
	<importlibrary definition="kbdurs.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdurs.c</file>
	<file>kbdurs.rc</file>
</module>
