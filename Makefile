CXX=g++
CC=gcc
CXXFLAGS=-std=c++14 -I./sfs_code 
CFLAGS=-I./sfs_code  
LDFLAGS=
SOURCES_C=$(wildcard sfs_code/*.c)
OBJECTS_C=$(SOURCES_C:.c=.o)
SOURCES_CPP=dils.cpp
OBJECTS_CPP=$(SOURCES_CPP:.cpp=.o)
EXECUTABLE=dils

all: $(SOURCES_C) $(SOURCES_CPP) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS_C) $(OBJECTS_CPP)
	$(CXX) $(LDFLAGS) $(OBJECTS_C) $(OBJECTS_CPP) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

fullclean:
	rm -f $(EXECUTABLE) $(OBJECTS_C) $(OBJECTS_CPP)
clean:
	rm -f $(OBJECTS_C) $(OBJECTS_CPP)