<module name="opengl32" type="win32dll" baseaddress="${BASEADDRESS_OPENGL32}" installbase="system32" installname="opengl32.dll" unicode="yes" crt="msvcrt">
	<importlibrary definition="opengl32.spec" />
	<library>ntdll</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>glu32</library>
	<pch>opengl32.h</pch>
	<file>font.c</file>
	<file>gl.c</file>
	<file>opengl32.c</file>
	<file>wgl.c</file>
</module>
