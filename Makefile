CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -fPIC
LDFLAGS_LIB = -shared -fPIC
LDFLAGS_APP = -L$(BUILD_DIR) -Wl,-rpath,'$$ORIGIN'
TARGET_LIB = liblogger.so
TARGET_APP = logger_app
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

SOURCES_LIB = $(SRC_DIR)/logger.cpp
SOURCES_APP = $(SRC_DIR)/main.cpp
OBJECTS_LIB = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES_LIB))
OBJECTS_APP = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES_APP))

.PHONY: all clean lib app

all: lib app

lib: $(BUILD_DIR)/$(TARGET_LIB)

app: $(BUILD_DIR)/$(TARGET_APP)

$(BUILD_DIR)/$(TARGET_LIB): $(OBJECTS_LIB)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS_LIB) -o $@ $^

$(BUILD_DIR)/$(TARGET_APP): $(OBJECTS_APP)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS_APP) -llogger -pthread

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: all
	LD_LIBRARY_PATH=$(BUILD_DIR) $(BUILD_DIR)/$(TARGET_APP) log.txt INFO
