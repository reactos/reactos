<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdus" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdus.dll">
	<importlibrary definition="kbdus.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdus.c</file>
	<file>kbdus.rc</file>
</module>
