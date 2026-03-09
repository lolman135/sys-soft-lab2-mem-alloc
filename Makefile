NAME := allocator
NAME_TEST := test_$(NAME)
BUILD := build
SRC := src
LIB := $(SRC)/lib
CFLAGS := -Wall -g -I

MAIN_SRC   := $(SRC)/main.c
ALLOC_SRC  := $(LIB)/alloc.c
ALLOC_HDR  := $(LIB)/alloc.h
TEST_SRC   := $(SRC)/test.c

all: clean proto_build 


clean:
	rm -rf $(BUILD)

#TODO: delete later
proto_build:
	mkdir -p $(BUILD)
	gcc $(MAIN_SRC) -o $(BUILD)/$(NAME)


build:
	mkdir -p $(BUILD)
	gcc $(CFLAGS) $(LIB) $(MAIN_SRC) $(ALLOC_SRC) -o $(BUILD)/$(NAME)

build_test:
	mkdir -p $(BUILD)
	gcc $(CFLAGS) $(LIB) $(TEST_SRC) $(ALLOC_SRC) -o $(BUILD)/$(NAME_TEST)


run: clean build
	./$(BUILD)/$(NAME)

test: clean build_test
	./$(BUILD)/$(NAME_TEST)
