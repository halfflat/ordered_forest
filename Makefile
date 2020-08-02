.PHONY: clean realclean all coverage-report
.PRECIOUS: %.o

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

CPPFLAGS+=-I$(top)include
CXXFLAGS+=-std=c++14 -g

vpath %.cc $(top)test
vpath %.h $(top)include

all:: unit

unit.o: ordered_forest.h
unit: unit.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

coverage.info:
	$(MAKE) CXXFLAGS="--coverage -std=c++14 -g -O0" unit
	./unit > /dev/null
	lcov --capture --no-external --directory . --base-directory=$(top)include -o $@

coverage-report: coverage.info
	mkdir -p report
	genhtml -o report $<

clean:
	$(RM) unit.o unit.gcda unit.gcno ordered_forest.gcov coverage.info

realclean: clean
	$(RM) unit unit-instrumented
