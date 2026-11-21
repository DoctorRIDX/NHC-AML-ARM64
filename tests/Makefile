# Host-side unit tests (no Android, no AML). Needs only g++.
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter -fsanitize=address,undefined -I../include -I.
SRC = ../src/resolver.cpp ../src/compat.cpp ../src/core.cpp ../src/player.cpp ../src/vehicles.cpp \
      ../src/weapons.cpp ../src/settings.cpp ../src/safe_read.cpp fake_platform.cpp test_nhc.cpp

test: test_nhc
	./test_nhc

test_nhc: $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ -pthread

clean:
	rm -f test_nhc
