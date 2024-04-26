CXX=g++
CC=gcc
CXXFLAGS=-std=c++14 -I./sfs_code 
CFLAGS=-I./sfs_code  
LDFLAGS=
SOURCES_C=$(wildcard sfs_code/*.c)
OBJECTS_C=$(SOURCES_C:.c=.o)
SOURCES_CPP=dils.cpp sfs_code/fs.cpp dicpo.cpp dicpi.cpp
OBJECTS_CPP=$(SOURCES_CPP:.cpp=.o)
EXECUTABLE_DILS=dils
EXECUTABLE_DICPO=dicpo
EXECUTABLE_DICPI=dicpi

all: $(SOURCES_C) $(SOURCES_CPP) $(EXECUTABLE_DILS) $(EXECUTABLE_DICPO) $(EXECUTABLE_DICPI) clean

$(EXECUTABLE_DILS): $(OBJECTS_C) dils.o sfs_code/fs.o
	$(CXX) $(LDFLAGS) $(OBJECTS_C) dils.o sfs_code/fs.o -o $@

$(EXECUTABLE_DICPO): $(OBJECTS_C) dicpo.o sfs_code/fs.o 
	$(CXX) $(LDFLAGS) $(OBJECTS_C) dicpo.o sfs_code/fs.o -o $@

$(EXECUTABLE_DICPI): $(OBJECTS_C) dicpi.o sfs_code/fs.o 
	$(CXX) $(LDFLAGS) $(OBJECTS_C) dicpi.o sfs_code/fs.o -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

wipe:
	rm -f $(EXECUTABLE_DILS) $(EXECUTABLE_DICPO) $(EXECUTABLE_DICPI) $(OBJECTS_C) $(OBJECTS_CPP)

clean:
	rm -f $(OBJECTS_C) $(OBJECTS_CPP)
