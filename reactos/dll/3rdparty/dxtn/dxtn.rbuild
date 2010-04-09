<module name="dxtn" type="win32dll" entrypoint="0" installbase="system32" installname="dxtn.dll" allowwarnings="true" crt="msvcrt">
	<importlibrary definition="dxtn.spec" />
	<if property="ARCH" value="amd64">
		<!-- Gross hack to work around broken autoexport -->
		<define name="dllexport">aligned(1)</define>
	</if>
	<include base="dxtn">.</include>
	<file>fxt1.c</file>
	<file>dxtn.c</file>
	<file>wrapper.c</file>
	<file>texstore.c</file>
</module>
