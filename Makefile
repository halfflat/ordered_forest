.PHONY: clean realclean all

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

CPPFLAGS+=-I$(top)include
CXXFLAGS+=-std=c++14 -g

vpath %.cc $(top)test
vpath %.h $(top)include

all:: unit

unit: unit.cc ordered_forest.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) unit.o

realclean: clean
	$(RM) unit
