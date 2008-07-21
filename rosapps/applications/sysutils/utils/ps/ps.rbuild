<module name="ps" type="win32cui" installbase="bin" installname="ps.exe">
    <include base="ps">.</include>
    <library>user32</library>
    <library>kernel32</library>
    <library>ntdll</library>
    <file>ps.c</file>
</module>