<group>
<module name="jscript" type="win32dll" baseaddress="${BASEADDRESS_JSCRIPT}" installbase="system32" installname="jscript.dll" allowwarnings="true" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="jscript.spec" />
	<include base="jscript">.</include>
	<include base="jscript" root="intermediate">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<dependency>jsglobal</dependency>
	<library>wine</library>
	<library>kernel32</library>
	<library>oleaut32</library>
	<file>dispex.c</file>
	<file>engine.c</file>
	<file>jscript.c</file>
	<file>jscript_main.c</file>
	<file>jsutils.c</file>
	<file>lex.c</file>
	<file>parser.tab.c</file>
	<file>math.c</file>
	<file>number.c</file>
	<file>object.c</file>
	<file>regexp.c</file>
	<file>string.c</file>
	<file>array.c</file>
	<file>bool.c</file>
	<file>function.c</file>
	<file>global.c</file>
	<file>rsrc.rc</file>
</module>
<module name="jsglobal" type="embeddedtypelib">
	<dependency>stdole2</dependency>
	<file>jsglobal.idl</file>
</module>
</group>