<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdsk" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdsk.dll">
	<importlibrary definition="kbdsk.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdsk.c</file>
	<file>kbdsk.rc</file>
</module>
