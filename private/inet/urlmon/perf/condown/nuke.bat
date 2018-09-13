@echo Nuking condown ...
@delnode /q obj
@delnode /q objd
@delnode /q objp
@del /q/s build*.log 1>nul: 2>nul:
@del /q/s build*.err 1>nul: 2>nul:
@del /q/s build*.wrn 1>nul: 2>nul:
@echo Files remaining after nuke:
@dir *.* /a:-r-d/s/b
