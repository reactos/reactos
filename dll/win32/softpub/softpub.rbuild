<module name="softpub" type="win32dll" baseaddress="${BASEADDRESS_SOFTPUB}" installbase="system32" installname="softpub.dll" allowwarnings="true" entrypoint="0">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="softpub.spec" />
	<include base="softpub">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>wintrust</library>
	<file>softpub.rc</file>
</module>
