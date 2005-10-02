doxygen Doxyfile
cmd /c start /b /low /wait hhc doxy-doc\html\index.hhp
move /y doxy-doc\html\index.chm winefile.chm
