<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sol" type="win32gui" installbase="system32" installname="sol.exe" unicode="yes">
	<include base="sol">.</include>
	<include base="cardlib">.</include>
	<library>cardlib</library>
	<file>solcreate.cpp</file>
	<file>solgame.cpp</file>
	<file>solitaire.cpp</file>
	<file>rsrc.rc</file>
	<pch>solitaire.h</pch>
</module>
