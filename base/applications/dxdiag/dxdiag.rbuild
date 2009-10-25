<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="dxdiag" type="win32gui" installbase="system32" installname="dxdiag.exe" unicode="yes">
	<include base="dxdiag">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>version</library>
	<library>dinput8</library>
	<library>setupapi</library>
	<library>dxguid</library>
	<library>dsound</library>
	<library>ddraw</library>
	<library>version</library>
	<library>gdi32</library>
	<library>winmm</library>
	<library>wintrust</library>
	<library>d3d9</library>
	<file>system.c</file>
	<file>display.c</file>
	<file>sound.c</file>
	<file>music.c</file>
	<file>input.c</file>
	<file>network.c</file>
	<file>help.c</file>
	<file>dxdiag.c</file>
	<file>dxdiag.rc</file>
	<file>ddtest.c</file>
	<file>d3dtest.c</file>
	<file>d3dtest7.c</file>
	<file>d3dtest8.c</file>
	<file>d3dtest9.c</file>
	<pch>precomp.h</pch>
</module>
