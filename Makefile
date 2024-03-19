CXX := clang++
CXXFLAGS := -std=c++17 $(shell sdl2-config --cflags) -I/usr/local/include -I./
SDL_STATIC_LIBS := $(shell sdl2-config --static-libs)
LDFLAGS := $(SDL_STATIC_LIBS)

SRC_DIR := ./src
BUILD_DIR := ./build

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
EXECUTABLE := mainApp

.PHONY: all clean

all: $(BUILD_DIR) $(BUILD_DIR)/$(EXECUTABLE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
