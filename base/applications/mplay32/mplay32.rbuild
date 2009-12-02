<module name="mplay32" type="win32gui" installbase="system32" installname="mplay32.exe" unicode="yes">
	<include base="mplay32">.</include>
	<library>advapi32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>winmm</library>
	<library>shell32</library>
	<file>mplay32.c</file>
	<file>mplay32.rc</file>
</module>
