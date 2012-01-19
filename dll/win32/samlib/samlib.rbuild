<module name="samlib" type="win32dll" baseaddress="${BASEADDRESS_SAMLIB}" installbase="system32" installname="samlib.dll">
	<importlibrary definition="samlib.spec" />
	<include base="samlib">.</include>
	<library>ntdll</library>
	<library>advapi32</library>
	<file>dllmain.c</file>
	<file>samlib.c</file>
	<file>samlib.rc</file>
	<pch>precomp.h</pch>
</module>
