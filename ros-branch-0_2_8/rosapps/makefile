all: 
	@echo To build "rosapps" there are two paths:
	@echo ---
	@echo 1. copy the rosapps folder into the reactos\modules folder
	@echo 2. link reactos/modules/rosapps to rosapps
	@echo    UNIX
	@echo         cd $${ROS_SRC_ROOT}/reactos/modules
	@echo         ln -s $${ROS_SRC_ROOT}/rosapps rosapps
	@echo    WINDOWS
	@echo         cd %%ROS_SRC_ROOT%%\reactos\modules
	@echo         junction rosapps %%ROS_SRC_ROOT%%\rosapps
	@echo ---
	@echo Eventually you can run "make depends" in the \reactos directory to compile it.
