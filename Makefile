CXX = g++
CXXFLAGS = -std=c++20 -g 

SOURCES := $(wildcard */*.cc) $(wildcard *.cc)
OBJECTS := $(patsubst %.cc,%.o,$(SOURCES))
DEPENDS := $(patsubst %.cc,%.d,$(SOURCES))

WARNING := -Wall -Wextra

.PHONY: all clean

all: puipui

clean:
		$(RM) $(OBJECTS) $(DEPENDS) puipui

puipui: $(OBJECTS)
		$(CXX) $(CXXFLAGS) $^ -o $@

-include $(DEPENDS)

%.o: %.cc Makefile
		$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@


