<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="explorer" type="win32gui" installname="explorer.exe" allowwarnings="true" stdlib="host" usewrc="false">
	<linkerflag>-fexceptions</linkerflag>
	<include base="explorer">.</include>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="WIN32" />
	<define name="_ROS_" />
	<define name="_WIN32_IE">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="WINVER">0x0500</define>
	<define name="__WINDRES__" />
	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>ws2_32</library>
	<library>msimg32</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>shell32</library>
	<library>uuid</library>
	<library>notifyhook</library>
	<pch>precomp.h</pch>
	<directory name="desktop">
		<file>desktop.cpp</file>
	</directory>
	<directory name="dialogs">
		<file>searchprogram.cpp</file>
		<file>settings.cpp</file>
	</directory>
	<directory name="shell">
		<file>entries.cpp</file>
		<file>fatfs.cpp</file>
		<file>filechild.cpp</file>
		<file>shellfs.cpp</file>
		<file>mainframe.cpp</file>
		<file>ntobjfs.cpp</file>
		<file>pane.cpp</file>
		<file>regfs.cpp</file>
		<file>shellbrowser.cpp</file>
		<file>unixfs.cpp</file>
		<file>webchild.cpp</file>
		<file>winfs.cpp</file>
	</directory>
	<directory name="services">
		<file>shellservices.cpp</file>
		<file>startup.c</file>
	</directory>
	<directory name="taskbar">
		<file>desktopbar.cpp</file>
		<file>favorites.cpp</file>
		<file>taskbar.cpp</file>
		<file>startmenu.cpp</file>
		<file>traynotify.cpp</file>
		<file>quicklaunch.cpp</file>
	</directory>
	<directory name="utility">
		<file>shellclasses.cpp</file>
		<file>utility.cpp</file>
		<file>window.cpp</file>
		<file>dragdropimpl.cpp</file>
		<file>shellbrowserimpl.cpp</file>
		<file>xmlstorage.cpp</file>
		<file>xs-native.cpp</file>
	</directory>
	<file>explorer.cpp</file>
	<file>i386-stub-win32.c</file>
	<file>explorer.rc</file>
</module>
<installfile base=".">explorer-cfg-template.xml</installfile>
<directory name="notifyhook">
	<xi:include href="notifyhook/notifyhook.rbuild" />
</directory>
</group>
