<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sol" type="win32gui" installbase="system32" installname="sol.exe" unicode="no" allowwarnings="true">
	<include base="sol">.</include>
	<include base="sol">cardlib</include>
	<linkerflag>-lstdc++</linkerflag>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<family>applications</family>
	<family>guiapplications</family>
	<family>games</family>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<file>solcreate.cpp</file>
	<file>solgame.cpp</file>
	<file>solitaire.cpp</file>
	<directory name="cardlib">
		<file>cardbitmaps.cpp</file>
		<file>cardbutton.cpp</file>
		<file>cardcolor.cpp</file>
		<file>cardcount.cpp</file>
		<file>cardlib.cpp</file>
		<file>cardregion.cpp</file>
		<file>cardrgndraw.cpp</file>
		<file>cardrgnmouse.cpp</file>
		<file>cardstack.cpp</file>
		<file>cardwindow.cpp</file>
		<file>dropzone.cpp</file>
	</directory>
	<file>rsrc.rc</file>
</module>
