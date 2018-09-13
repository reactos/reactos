# project.inc - project specific settings

CTRL_INC_DIRS=$(SRCROOT)\ctrl\base $(SRCROOT)\ctrl\include $(SRCROOT)\core\include
CTRL_INC_DIRS=$(CTRL_INC_DIRS) $(SRCROOT)\site\include $(SRCROOT)\f3\include $(SDKDIR)
CTRL_INC_DIRS=$(CTRL_INC_DIRS) $(TOOLSINCLUDE) $(TOOLSMISCINCLUDE) $(BLDROOT)\types
CTRL_INC_DIRS=$(CTRL_INC_DIRS) $(SRCROOT)\..\tools\riched\inc $(ROOT)\help

