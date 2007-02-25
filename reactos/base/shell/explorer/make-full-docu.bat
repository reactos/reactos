doxygen Doxyfile
cmd /c start /b /low /wait hhc doxy-doc\html\index.hhp
cmd /c move /y doxy-doc\html\index.chm ros-explorer.chm

doxygen Doxyfile-all
cmd /c start /b /low /wait hhc doxy-doc\html\index.hhp
cmd /c move /y doxy-doc\html\index.chm ros-explorer-full.chm
