all: 
	@echo To build "posix" there are two paths:
	@echo ---
	@echo 1. copy the posix folder into the reactos\modules folder
	@echo 2. link reactos/modules/posix to posix
	@echo    UNIX
	@echo         cd $${ROS_SRC_ROOT}/reactos/modules
	@echo         ln -s $${ROS_SRC_ROOT}/posix posix
	@echo    WINDOWS
	@echo         cd %%ROS_SRC_ROOT%%\reactos\modules
	@echo         junction posix %%ROS_SRC_ROOT%%\posix
	@echo ---
	@echo Eventually you can run "make depends" in the %%ROS_SRC_ROOT%%\reactos
	@echo directory to compile it.
