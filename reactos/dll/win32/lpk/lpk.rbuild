<module name="lpk" type="win32dll" baseaddress="${BASEADDRESS_LPK}" installbase="system32" installname="lpk.dll" unicode="yes">
	<importlibrary definition="lpk.spec" />
	<include base="lpk">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="LANGPACK" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>usp10</library>

	<file>dllmain.c</file>
	<file>stub.c</file>

	<file>lpk.rc</file>
</module>
