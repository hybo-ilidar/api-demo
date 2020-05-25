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
ifeq ($(THIS_OS),WIN32)
	PATHSEP2=\\
  SLASH=$(strip $(PATHSEP2))
	CC=cl
	CPP=cl
  CPPVER=/std:c++17
	RM=del /f /q
  EXESWITCH=/Fe:
	EXE=.exe
  ERR2NULL=2>nul
  MKDIR=mkdir
	OBJLOC=/Fo: $(BUILD)$(SLASH) 
	INCLUDES=/I $(INC)
  LIBSWITCH=
	#LIBSERIAL=$(LIBPROJ)$(SLASH)serial$(SLASH)libserialport.lib
	LIBSERIAL=..$(SLASH)serial-sigrok$(SLASH)x64$(SLASH)Release$(SLASH)libserialport.lib
else
	PATHSEP2=/
  SLASH=$(strip $(PATHSEP2))
  CC=gcc
	CPP=g++
  RM=rm -f
	EXE=
  EXESWITCH=-o
  ERR2NULL=
  MKDIR=mkdir -p
	OBJLOC=
	INCLUDES=-I$(INC)
  LIBSWITCH=-l
	LIBSERIAL=serialport
endif


.PHONY: directories

all: directories logsensor rdsensor sensor2csv # rdsensor_t

directories: $(BIN) $(BUILD)

logsensor: $(SRC)/logsensor.c $(SRC)/ilidarlib.c  | $(BUILD)
	$(CC) $(INCLUDES) $(OBJLOC) $(SRC)$(SLASH)logsensor.c $(SRC)$(SLASH)ilidarlib.c $(LIBSWITCH)$(LIBSERIAL) \
		$(EXESWITCH) $(BIN)$(SLASH)logsensor$(EXE)

rdsensor: $(SRC)/rdsensor.c $(SRC)/ilidarlib.c  | $(BUILD)
	$(CC) $(INCLUDES) $(OBJLOC) $(SRC)$(SLASH)rdsensor.c $(SRC)$(SLASH)ilidarlib.c $(LIBSWITCH)$(LIBSERIAL) \
		$(EXESWITCH) $(BIN)$(SLASH)rdsensor$(EXE)

sensor2csv: $(SRC)/sensor2csv.c $(SRC)/ilidarlib.c  | $(BUILD)
	$(CC) $(INCLUDES) $(OBJLOC) $(SRC)$(SLASH)sensor2csv.c $(SRC)$(SLASH)ilidarlib.c $(LIBSWITCH)$(LIBSERIAL) \
		$(EXESWITCH) $(BIN)$(SLASH)sensor2csv$(EXE)

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

