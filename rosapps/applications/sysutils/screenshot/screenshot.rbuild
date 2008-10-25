<module name="screenshot" type="win32gui" installbase="system32" installname="screenshot.exe" unicode="yes">
	<include base="screenshot">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comdlg32</library>
	<file>screenshot.c</file>
	<file>screenshot.rc</file>
</module>
