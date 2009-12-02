<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="mment4" type="staticlibrary" allowwarnings="false" unicode="yes">
	<include base="ReactOS">include/reactos/libs/sound</include>
	<define name="DEBUG_NT4">1</define>
	<file>detect.c</file>
	<file>registry.c</file>
    <file>control.c</file>
</module>
