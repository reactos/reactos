<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="rapps" type="win32gui" installbase="system32" installname="rapps.exe" unicode="yes">
	<include base="ReactOS">include/reactos</include>
	<include base="rapps" root="intermediate">.</include>
	<include base="rapps">.</include>

	<library>advapi32</library>
	<library>comctl32</library>
	<library>gdi32</library>
	<library>urlmon</library>
	<library>user32</library>
	<library>uuid</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<library>ntdll</library>

	<dependency>rappsmsg</dependency>

	<file>aboutdlg.c</file>
	<file>available.c</file>
	<file>installdlg.c</file>
	<file>installed.c</file>
	<file>listview.c</file>
	<file>loaddlg.c</file>
	<file>misc.c</file>
	<file>parser.c</file>
	<file>richedit.c</file>
	<file>settingsdlg.c</file>
	<file>splitter.c</file>
	<file>statusbar.c</file>
	<file>toolbar.c</file>
	<file>treeview.c</file>
	<file>winmain.c</file>
	<file>rapps.rc</file>
</module>
<module name="rappsmsg" type="messageheader">
	<file>rappsmsg.mc</file>
</module>
</group>
