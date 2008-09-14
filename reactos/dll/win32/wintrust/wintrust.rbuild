<module name="wintrust" type="win32dll" baseaddress="${BASEADDRESS_WINTRUST}" installbase="system32" installname="wintrust.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="wintrust.spec.def" />
	<include base="wintrust">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>crypt32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>imagehlp</library>
	<library>ntdll</library>
	<file>crypt.c</file>
	<file>register.c</file>
	<file>wintrust_main.c</file>
	<file>asn.c</file>
	<file>softpub.c</file>
	<file>version.rc</file>
	<file>wintrust.spec</file>
</module>
