#include CommonDefs.mak
OPENNI2_DIR=/home/amir/Downloads/OpenNI/OpenNI-Linux-x64-2.2/
OPENNI2_INCLUDE=$(OPENNI2_DIR)/Include
OPENNI2_REDIST=$(OPENNI2_DIR)/Redist
BIN_DIR = Bin

INC_DIRS = \
	../../Include \
	../Common
CXX=g++-4.8
SRC_FILES = *.cpp
objects = $(patsubst src/%.cpp, build/%.o, $(wildcard src/*.cpp))
USED_LIBS += OpenNI2

#EXE_NAME = SimpleRead

Q=@

CFLAGS += -Wall -g -std=c++0x -O2
CFLAGS += -I"$(OPENNI2_INCLUDE)"
CFLAGS += -Iinclude 
LDFLAGS += -L/usr/local/lib -lopencv_highgui -lopencv_imgproc -lopencv_core -lopencv_imgcodecs -pthread 
LDFLAGS += -L$(OPENNI2_REDIST) -lOpenNI2 
LDFLAGS += -larmadillo
#ifndef OPENNI2_INCLUDE
#    $(error OPENNI2_INCLUDE is not defined. Please define it or 'source' the OpenNIDevEnvironment file from the installation)
#else ifndef OPENNI2_REDIST
#    $(error OPENNI2_REDIST is not defined. Please define it or 'source' the OpenNIDevEnvironment file from the installation)
#endif

INC_DIRS += $(OPENNI2_INCLUDE)

#####include CommonCppMakefile
#all:	elf
all:	$(objects) build/out.elf
build/%.o: src/%.cpp
	@printf "CC\t$@\n"
	$(Q)$(CXX) $(CFLAGS) $(LDFLAGS) -c -o $@  src/$*.cpp
build/out.elf: $(objects)
	@printf "LD\t$@\n"
	$(Q)$(CXX) $(CFLAGS) -o $@ $(objects) $(LDFLAGS)
exportlibpaths:
	echo export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(OPENNI2_REDIST)
clean:
	rm -f build/*.o
	rm -f build/out.elf
.PHONY: all clean exportlibpaths *.o

