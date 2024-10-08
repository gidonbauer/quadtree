TARGETS = ${basename ${wildcard *.cpp}}
CXX_FLAGS = -Wall -Wextra -pedantic -Wconversion -Wshadow -march=native -std=c++20

INC = -I./include

all: CXX_FLAGS += -O3
all: ${TARGETS}

debug: CXX_FLAGS += -O0 -g -fsanitize=address,undefined
debug: ${TARGETS}

%: %.cpp
	${CXX} ${CXX_FLAGS} ${INC} -o $@ $<

clean:
	rm -fr ${TARGETS} ${addsuffix .dSYM, ${TARGETS}}

.PHONY: all debug clean

