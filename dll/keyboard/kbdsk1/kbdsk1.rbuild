<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdsk1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdsk1.dll">
	<importlibrary definition="kbdsk1.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdsk1.c</file>
	<file>kbdsk1.rc</file>
</module>
