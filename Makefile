CC := g++
SRCS := $(shell find ./* -type f | grep "\.cpp"|grep -v "./main\.cpp")
$(warning SRCS is $(SRCS))


OBJS := $(patsubst %.cpp, %.o, $(filter %.cpp, $(SRCS)))
$(warning OBJS is $(OBJS))

CFLAGS := -g -O2 -Wall -Wno-unused -ldl -std=c++11
INCLUDE := -I.

MAIN_SRC:=main.cpp
MAIN_OBJ:=$(MAIN_SRC: %.cpp=%.o)
MAIN_EXE:=a.out

target: $(MAIN_EXE)
	
$(MAIN_EXE):$(OBJS) $(MAIN_OBJ)
	$(CC) -o $@ $^ $(INCLUDE) $(CFLAGS)
%.o: %.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)  a.out
