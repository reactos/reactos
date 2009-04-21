<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbddv" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbddv.dll">
	<importlibrary definition="kbddv.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbddv.c</file>
	<file>kbddv.rc</file>
</module>
