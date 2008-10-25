<module name="template_dialog" type="win32cui" installname="dialog.exe" allowwarnings="true">
	<include base="template_dialog">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
 	<file>dialog.c</file>
 	<file>memdlg.c</file>
	<file>page1.c</file>
	<file>page2.c</file>
	<file>page3.c</file>
	<file>trace.c</file>
</module>
