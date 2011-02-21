<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="spider" type="win32gui" installbase="system32" installname="spider.exe" unicode="yes">
	<include base="spider">.</include>
    <include base="cardlib">.</include>
	<library>cardlib</library>
	<file>spigame.cpp</file>
	<file>spider.cpp</file>
	<file>rsrc.rc</file>
</module>
