<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdru" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdru.dll">
	<importlibrary definition="kbdru.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdru.c</file>
	<file>kbdru.rc</file>
</module>
