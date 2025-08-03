CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -fPIC
LDFLAGS := -shared -fPIC
TARGET_LIB := liblogger.so
TARGET_APP := logger_app
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

SOURCES_LIB := $(SRC_DIR)/logger.cpp
SOURCES_APP := $(SRC_DIR)/main.cpp
OBJECTS_LIB := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES_LIB))
OBJECTS_APP := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES_APP))

.PHONY: all clean lib app

all: lib app

lib: $(BUILD_DIR)/$(TARGET_LIB)

app: $(BUILD_DIR)/$(TARGET_APP)
#	rm -rf $(BUILD_DIR)/$(TARGET_LIB)
#	find . -name "*.o" -exec rm -rf {} \;

$(BUILD_DIR)/$(TARGET_LIB): $(OBJECTS_LIB)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/$(TARGET_APP): $(OBJECTS_APP)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ -L$(BUILD_DIR) -llogger -pthread

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: all
	LD_LIBRARY_PATH=$(BUILD_DIR) $(BUILD_DIR)/$(TARGET_APP) log.txt INFO


# WWW_FLAGS=-Wall -Werror -Wextra
# GTEST_LIBS_FLAGS=-lgtest -lgtest_main -pthread
# FLAGS=-lstdc++ -static-libgcc -static-libstdc++
# CPP_FLAGS=-std=c++17
# флаг для отладки
# G_FLAG=-g

# убрал test после clean
# all: clean run

# TEST_SRC = $(wildcard tests/*.cpp)
# test:
#	 g++ $(WWW_FLAGS) -g $(TEST_SRC) -std=c++20 -o test $(GTEST_LIBS_FLAGS)
	
# добавил test
# run: test
#	./test

# TEST_O = $(wildcard tests/*.o)
# clean:
#	rm -f test

# добавил test
# vg: clean test
#	valgrind --tool=memcheck -s --leak-check=yes ./test

# leaks: clean test
#	leaks -atExit -- ./test