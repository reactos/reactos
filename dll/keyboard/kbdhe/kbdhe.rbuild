<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdhe" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdhe.dll">
	<importlibrary definition="kbdhe.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdhe.c</file>
	<file>kbdhe.rc</file>
</module>
