<module name="packmgr" type="win32gui" installbase="system32" installname="packmgr.exe">
	<include base="package">.</include>
	<include base="packmgr">.</include>
	<define name="UNICODE" />

	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>package</library>

	<file>main.c</file>
	<file>packmgr.rc</file>
</module>
