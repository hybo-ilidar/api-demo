# Detect operating system in Makefile.
# modified from here: https://gist.github.com/sighingnow/deee806603ec9274fd47
ifeq ($(OS),Windows_NT)
	THIS_OS = WIN32
	ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
		THIS_ARCH = AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITEW6432),x86)
		THIS_ARCH = IA32
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		THIS_OS = LINUX
	endif
	ifeq ($(UNAME_S),Darwin)
		THIS_OS += OSX
	endif
	UNAME_M := $(shell uname -m)
	ifeq ($(UNAME_M),x86_64)
		THIS_ARCH = AMD64
	endif
	ifneq ($(filter %86,$(UNAME_M)),)
	  THIS_ARCH = IA32
	endif
	ifneq ($(filter arm%,$(UNAME_M)),)
		THIS_ARCH += ARM
	endif
endif

#all:
#	@echo $(THIS_OS)  $(THIS_ARCH)

# Note to self:
# https://ftp.gnu.org/old-gnu/Manuals/make-3.79.1/html_chapter/make_10.html
# http://skramm.blogspot.com/2013/04/writing-portable-makefiles.html
SRC = src
INC = include
BIN = bin
BUILD = build
LIBPROJ = lib

LIBS = 
CCFLAGS =
INCLUDES =
DEFINES =

ifeq ($(THIS_OS),WIN32)
  COMPILE=/c
  PATHSEP2=\\
  SLASH=$(strip $(PATHSEP2))
  CC=cl
  CPP=cl
  CPPVER=/std:c++17
  RM=del /f /q
  EXESWITCH=/Fe:
  EXE=.exe
  OBJ=.obj
  ERR2NULL=2>nul
  MKDIR=mkdir
  OBJLOC=/Fo:
  INCLUDES=/I $(INC)
  SRC_GETOPT=ya_getopt.c
  LIBS = ..\serial-sigrok\x64\Release\libserialport.lib





else
  COMPILE=-c
  PATHSEP2=/
  SLASH=$(strip $(PATHSEP2))
  CC=gcc
  CPP=g++
  RM=rm -f
  EXE=
  OBJ=.o
  EXESWITCH=-o
  ERR2NULL=
  MKDIR=mkdir -p
  OBJLOC=-o
  INCLUDES=-I$(INC)
  SRC_GETOPT=
  LIBS += -L/usr/local/lib -lserialport
endif

SOURCES  = logsensor.c 
SOURCES += ilidarlib.c
OBJS_LOGSENSOR := $(SOURCES:%.c=$(BUILD)$(SLASH)%$(OBJ))
$(info SOURCES is $(SOURCES))
$(info OBJS_LOGSENSOR is $(OBJS_LOGSENSOR))

SOURCES  = rdsensor.c 
SOURCES += ilidarlib.c
OBJS_RDSENSOR := $(SOURCES:%.c=$(BUILD)$(SLASH)%$(OBJ))
$(info SOURCES is $(SOURCES))
$(info OBJS_RDSENSOR is $(OBJS_RDSENSOR))

SOURCES  = sensor2csv.c 
SOURCES += ilidarlib.c
SOURCES += $(SRC_GETOPT)
OBJS_SENSOR2CSV := $(SOURCES:%.c=$(BUILD)$(SLASH)%$(OBJ))
$(info SOURCES is $(SOURCES))
$(info OBJS_SENSOR2CSV is $(OBJS_SENSOR2CSV))

.PHONY: directories

all: directories logsensor rdsensor sensor2csv # rdsensor_t

directories: $(BIN) $(BUILD)


sensor2csv: $(OBJS_SENSOR2CSV) | $(BIN)
	$(CC) $^ $(EXESWITCH) $(BIN)$(SLASH)sensor2csv$(EXE) $(LIBS)

logsensor: $(OBJS_SENSOR2CSV) | $(BIN)
	$(CC) $^ $(EXESWITCH) $(BIN)$(SLASH)logsnsor$(EXE) $(LIBS)

rdsensor: $(OBJS_SENSOR2CSV) | $(BIN)
	$(CC) $^ $(EXESWITCH) $(BIN)$(SLASH)rdsensor$(EXE) $(LIBS)


$(BUILD)$(SLASH)%$(OBJ): $(SRC)$(SLASH)%.c | $(BUILD)
	$(CC) $(DEFINES) $(INCLUDES) $(OBJLOC) $@ $(COMPILE) $<


##  logsensor: $(SRC)/logsensor.c $(SRC)/ilidarlib.c  | $(BUILD)
##  	$(CC) $(INCLUDES) $(OBJLOC) $(SRC)$(SLASH)logsensor.c $(SRC)$(SLASH)ilidarlib.c $(LIBSWITCH)$(LIBSERIAL) \
##  		$(EXESWITCH) $(BIN)$(SLASH)logsensor$(EXE)
##  
##  rdsensor: $(SRC)/rdsensor.c $(SRC)/ilidarlib.c  | $(BUILD)
##  	$(CC) $(INCLUDES) $(OBJLOC) $(SRC)$(SLASH)rdsensor.c $(SRC)$(SLASH)ilidarlib.c $(LIBSWITCH)$(LIBSERIAL) \
##  		$(EXESWITCH) $(BIN)$(SLASH)rdsensor$(EXE)

### rdsensor_t: $(SRC)/rdsensor_t.cpp $(SRC)/ilidarlib.c  | $(BUILD)
### 	$(CPP) $(CPPVER) $(INCLUDES) $(OBJLOC) $(SRC)$(SLASH)rdsensor_t.cpp $(SRC)$(SLASH)ilidarlib.cpp \
### 		$(LIBSWITCH)$(LIBSERIAL) \
### 		$(EXESWITCH) $(BIN)$(SLASH)rdsensor_t$(EXE)

$(BIN):
	$(MKDIR) $(BIN)

$(BUILD):
	$(MKDIR) $(BUILD)

clean:
	$(RM) $(BIN)$(SLASH)logsensor$(EXE) $(ERR2NULL)
	$(RM) $(BIN)$(SLASH)rdsensor$(EXE) $(ERR2NULL)
	$(RM) $(BIN)$(SLASH)sensor2csv$(EXE) $(ERR2NULL)
	# $(RM) $(BIN)$(SLASH)rdsensor_t$(EXE) $(ERR2NULL)
	$(RM) $(BUILD)$(SLASH)*.obj $(ERR2NULL)
	$(RM) $(BUILD)$(SLASH)*.o $(ERR2NULL)

exec:
ifneq ($(THIS_OS),WIN32)
	chmod +x $(BIN)/logsensor
	chmod +x $(BIN)/rdsensor
	chmod +x $(BIN)/sensor2csv
	#chmod +x $(BIN)/rdsensor_t
endif

install:
ifneq ($(THIS_OS),WIN32)
	#TBD
endif

