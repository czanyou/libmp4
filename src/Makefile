# Copyright (c) 2014-2015 ChengZhen(Anyou). All Rights Reserved.
# Author: ChengZhen (anyou@msn.com)

include ../Makefile.rules

APP_NAME = mp4
LIB_NAME = lib$(APP_NAME).a
OBJS	:= $(COBJ) $(CXXOBJ)

all: $(APP_NAME)

mp4: $(OBJS)
	$(TOOL_AR) -rc $(LIB_NAME) $(OBJS)
#	$(CP) $(LIB_NAME) $(PROJECT_OUTPUT)/

clean:
	rm -f $(LIB_NAME) *.o $(PROJECT_OUTPUT)/$(LIB_NAME)

