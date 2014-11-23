all: 
	@echo To build "os2" there are two paths:
	@echo ---
	@echo 1. copy the os2 folder into the reactos\modules folder
	@echo 2. link reactos/modules/os2 to os2
	@echo    UNIX
	@echo         cd $${ROS_SRC_ROOT}/reactos/modules
	@echo         ln -s $${ROS_SRC_ROOT}/os2 os2
	@echo    WINDOWS
	@echo         cd %%ROS_SRC_ROOT%%\reactos\modules
	@echo         junction os2 %%ROS_SRC_ROOT%%\os2
	@echo ---
	@echo Eventually you can run "make depends" in the %%ROS_SRC_ROOT%%\reactos
	@echo directory to compile it.
