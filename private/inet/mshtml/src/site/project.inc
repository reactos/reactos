# project.inc - project specific settings

SITE_INC_DIRS = $(SRCROOT)\site\include $(SRCROOT)\core\include 
SITE_INC_DIRS = $(SITE_INC_DIRS) $(SRCROOT)\f3\include $(SDKDIR) 
SITE_INC_DIRS = $(SITE_INC_DIRS) $(TOOLSINCLUDE) $(TOOLSMISCINCLUDE) 
SITE_INC_DIRS = $(SITE_INC_DIRS) $(BLDROOT)\types $(SRCROOT)\core\types 
SITE_INC_DIRS = $(SITE_INC_DIRS) $(SRCROOT)\f3\types $(SRCROOT)\f3\ocx96
SITE_INC_DIRS = $(SITE_INC_DIRS) $(SRCROOT)\site\types
SITE_INC_DIRS = $(SITE_INC_DIRS) $(SRCROOT)\other\include


