all: 
	@echo To build "vms" there are two paths:
	@echo ---
	@echo 1. copy the vms folder into the reactos\modules folder
	@echo 2. link reactos/modules/vms to vms
	@echo    UNIX
	@echo         cd $${ROS_SRC_ROOT}/reactos/modules
	@echo         ln -s $${ROS_SRC_ROOT}/vms vms
	@echo    WINDOWS
	@echo         cd %%ROS_SRC_ROOT%%\reactos\modules
	@echo         junction vms %%ROS_SRC_ROOT%%\vms
	@echo ---
	@echo Eventually you can run "make depends" in the %%ROS_SRC_ROOT%%\reactos
	@echo directory to compile it.
