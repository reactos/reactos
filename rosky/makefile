all: 
	@echo To build "rosky" there are two paths:
	@echo ---
	@echo 1. copy the rosky folder into the reactos\modules folder
	@echo 2. link reactos/modules/rosky to rosky
	@echo    UNIX
	@echo         cd $${ROS_SRC_ROOT}/reactos/modules
	@echo         ln -s $${ROS_SRC_ROOT}/rosky rosky
	@echo    WINDOWS
	@echo         cd %%ROS_SRC_ROOT%%\reactos\modules
	@echo         junction rosky %%ROS_SRC_ROOT%%\rosky
	@echo ---
	@echo Eventually you can run "make depends" in the %%ROS_SRC_ROOT%%\reactos
	@echo directory to compile it.
