CXX=g++
CXXFLAGS=-Wall -Wextra  -O2
LDFLAGS= -lwiringPi
BUILD_DIR=build
SRC_DIR=src
EXECUTABLE=t2

PROJECT_SRCS=(src/main.cpp src/uart.cpp)
SRC_FILES=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(PROJECT_SRCS))
OBJ_FILES=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(PROJECT_SRCS))

all: $(BUILD_DIR) $(BUILD_DIR)/$(EXECUTABLE) $(BUILD_DIR)/exercicio1

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/$(EXECUTABLE): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/exercicio1 : src/exercicio1.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

debug: CXXFLAGS += -g -DDEBUG
debug: all

.PHONY: all clean debug
