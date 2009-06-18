<module name="wintrust" type="win32dll" baseaddress="${BASEADDRESS_WINTRUST}" installbase="system32" installname="wintrust.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="wintrust.spec" />
	<include base="wintrust">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>crypt32</library>
	<library>cryptui</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>imagehlp</library>
	<library>ntdll</library>
	<library>pseh</library>
	<file>crypt.c</file>
	<file>register.c</file>
	<file>wintrust_main.c</file>
	<file>asn.c</file>
	<file>softpub.c</file>
	<file>version.rc</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>
