-include $(TOPDIR)/Make.defs

#Set the scheduler priority for the app.
CONFIG_UROS_RBS_EXAMPLE_PRIORITY ?= SCHED_PRIORITY_DEFAULT
#Set the stack size to the app. The minimum stack size on NuttX for a micro-ROS App is 65000 bytes
CONFIG_UROS_RBS_EXAMPLE_STACKSIZE ?= 68640
# with CONFIG_SCHED_SPORADIC:
# 65000 => uros_rbs hangs after some while
# 68480 => uros_rbs hangs after some while
# 68600 => uros_rbs hangs after some while (after sending messages)
# 68625 => works!
# 68650 => nsh:uros_rbs: command not found
# 68700 => nsh:uros_rbs: command not found
# 68800 => nsh:uros_rbs: command not found
# 69000 => nsh:uros_rbs: command not found
# 
#This is the name of the app on the NSH console
APPNAME = uros_rbs
PRIORITY = $(CONFIG_UROS_RBS_EXAMPLE_PRIORITY)
STACKSIZE = $(CONFIG_UROS_RBS_EXAMPLE_STACKSIZE)

#Add the source files.
ASRCS =
CSRCS =
MAINSRC = main_rbs.c 

CONFIG_UROS_RBS_EXAMPLE_PROGNAME ?= uros_rbs$(EXEEXT)
PROGNAME = $(CONFIG_UROS_RBS_EXAMPLE_PROGNAME)
UROS_RBS_INCLUDES = $(shell find $(APPDIR)/$(CONFIG_UROS_DIR)/install -type d -name include)
CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" "$(UROS_RBS_INCLUDES)"} -std=c99

MODULE = CONFIG_UROS_RBS_EXAMPLE

include $(APPDIR)/Application.mk