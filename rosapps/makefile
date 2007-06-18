all: 
	@echo To build "rosapps" there are two paths:
	@echo ---
	@echo 1. copy the rosapps folder into the reactos\modules folder
	@echo 2. link reactos/modules/rosapps to rosapps
	
ifeq ($(shell uname),)
# Information for Windows users

	@echo         cd %%_ROSSOURCEDIR%%\reactos\modules
	@echo         junction rosapps %%_ROSSOURCEDIR%%\rosapps
else
# Information for UNIX users

	@echo "        cd \$$_ROSSOURCEDIR/reactos/modules"
	@echo "        ln -s \$$_ROSSOURCEDIR/rosapps rosapps"
endif
	
	@echo ---
	@echo Eventually you can run "make depends" in the \reactos directory to compile it.
