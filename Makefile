CXX=g++
CXXFLAGS=-Wall -Wextra  -O2
LDFLAGS= -lwiringPi
BUILD_DIR=build
SRC_DIR=src
EXECUTABLE=t2

SRC_FILES=$(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))

all: $(BUILD_DIR) $(BUILD_DIR)/$(EXECUTABLE)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/$(EXECUTABLE): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

debug: CXXFLAGS += -g -DDEBUG
debug: all

.PHONY: all clean debug
