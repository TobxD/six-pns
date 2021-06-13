CXX = g++ -std=c++11
OPT_FLAGS = -O2 -g
DEBUG_FLAGS = -g -Wall -Wextra -fsanitize=undefined -D_GLIBCXX_DEBUG -Wconversion -Wshadow -Wfloat-equal -pedantic -Werror

SOURCES = $(wildcard src/*.cpp)
OBJ = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))
OBJ_D = $(patsubst src/%.cpp, obj_d/%.do, $(SOURCES))
DEPS = $(wildcard src/*.h)

all: opt

obj/%.o: src/%.cpp $(DEPS)
	$(CXX) $(OPT_FLAGS) -c -o $@ $<

obj_d/%.do: src/%.cpp $(DEPS)
	$(CXX) $(DEBUG_FLAGS) -c -o $@ $<

opt: $(OBJ) $(DEPS)
	$(CXX) $(OPT_FLAGS) -o pns $(OBJ)

debug: $(OBJ_D) $(DEPS)
	$(CXX) $(DEBUG_FLAGS) -o pns $(OBJ_D)

init:
	mkdir -p obj
	mkdir -p obj_d

clean:
	rm -f pns
	rm -f obj/*.o
	rm -f obj_d/*.do
