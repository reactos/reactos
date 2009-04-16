<module name="Imagelistviewer" type="win32gui" installbase="bin" installname="Imagelistviewer.exe">
	<define name="__USE_W32API" />
	<include base="Imagelistviewer">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>setupapi</library>
	<library>comctl32</library>
	<file>main.c</file>
	<file>res.rc</file>
</module>
