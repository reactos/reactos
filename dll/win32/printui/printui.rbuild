<module name="printui" type="win32dll" baseaddress="${BASEADDRESS_PRINTUI}" installbase="system32" installname="printui.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="printui.spec" />
        <include base="printui">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <library>wine</library>
        <library>shell32</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>printui.c</file>
        <file>printui.rc</file>
</module>
