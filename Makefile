CXX=g++
CC=gcc
CXXFLAGS= -Wall -std=c++14 -I./sfs_code 
CFLAGS=-I./sfs_code  
LDFLAGS=
SOURCES_C=$(wildcard sfs_code/*.c)
OBJECTS_C=$(SOURCES_C:.c=.o)
SOURCES_CPP=dils.cpp dicpo.cpp dicpi.cpp fsck.cpp sfs_code/fs.cpp
OBJECTS_CPP=$(SOURCES_CPP:.cpp=.o)
EXECUTABLE_DILS=dils
EXECUTABLE_DICPO=dicpo
EXECUTABLE_DICPI=dicpi
EXECUTABLE_FSCK=fsck

all: $(SOURCES_C) $(SOURCES_CPP) $(EXECUTABLE_DILS) $(EXECUTABLE_DICPO) $(EXECUTABLE_DICPI) $(EXECUTABLE_FSCK) clean

$(EXECUTABLE_DILS): $(OBJECTS_C) dils.o sfs_code/fs.o
	$(CXX) $(LDFLAGS) $(OBJECTS_C) dils.o sfs_code/fs.o -o $@

$(EXECUTABLE_DICPO): $(OBJECTS_C) dicpo.o sfs_code/fs.o 
	$(CXX) $(LDFLAGS) $(OBJECTS_C) dicpo.o sfs_code/fs.o -o $@

$(EXECUTABLE_DICPI): $(OBJECTS_C) dicpi.o sfs_code/fs.o 
	$(CXX) $(LDFLAGS) $(OBJECTS_C) dicpi.o sfs_code/fs.o -o $@

$(EXECUTABLE_FSCK): $(OBJECTS_C) fsck.o sfs_code/fs.o 
	$(CXX) $(LDFLAGS) $(OBJECTS_C) fsck.o sfs_code/fs.o -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

wipe:
	rm -f $(EXECUTABLE_DILS) $(EXECUTABLE_DICPO) $(EXECUTABLE_DICPI) $(EXECUTABLE_FSCK) $(OBJECTS_C) $(OBJECTS_CPP)

clean:
	rm -f $(OBJECTS_C) $(OBJECTS_CPP)
