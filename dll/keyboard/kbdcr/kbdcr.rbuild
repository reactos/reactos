<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdcr" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdcr.dll">
	<importlibrary definition="kbdcr.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdcr.c</file>
	<file>kbdcr.rc</file>
</module>
