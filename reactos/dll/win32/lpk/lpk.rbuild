<module name="lpk" type="win32dll" baseaddress="${BASEADDRESS_LPK}" installbase="system32" installname="lpk.dll" unicode="yes">
	<importlibrary definition="lpk.def" />
	<include base="lpk">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="LANGPACK" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>usp10</library>

	<file>dllmain.c</file>
	<file>stub.c</file>

	<linkerflag>-lgcc</linkerflag>
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<file>lpk.rc</file>
</module>
