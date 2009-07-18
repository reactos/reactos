<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
  <module name="winfile" type="win32gui" installbase="system32" installname="winfile.exe" allowwarnings="true">
    <include base="winfile">.</include>
    <define name="UNICODE" />
    <library>shell32</library>
    <library>comdlg32</library>
    <library>comctl32</library>
    <library>ole32</library>
    <library>mpr</library>
    <library>version</library>
    <library>user32</library>
    <library>gdi32</library>
    <library>advapi32</library>
    <library>kernel32</library>
    <library>uuid</library>
    <file>splitpath.c</file>
    <file>winefile.c</file>
    <file>Cs.rc</file>
    <file>Da.rc</file>
    <file>De.rc</file>
    <file>En.rc</file>
    <file>Fr.rc</file>
    <file>Hu.rc</file>
    <file>It.rc</file>
    <file>Ja.rc</file>
    <file>Ko.rc</file>
    <file>Lt.rc</file>
    <file>Nl.rc</file>
    <file>No.rc</file>
    <file>Pl.rc</file>
    <file>Pt.rc</file>
    <file>Ru.rc</file>
    <file>Si.rc</file>
    <file>Sv.rc</file>
    <file>Tr.rc</file>
    <file>Zh.rc</file>
    <file>rsrc.rc</file>
  </module>
</rbuild>
