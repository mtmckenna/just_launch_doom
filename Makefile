CXX := clang++
CXXFLAGS := -std=c++17 $(shell sdl2-config --cflags) -I/usr/local/include -I./
SDL_STATIC_LIBS := $(shell sdl2-config --static-libs)
LDFLAGS := $(SDL_STATIC_LIBS)
DEBUGFLAGS := -g -O0
CXXFLAGS += $(DEBUGFLAGS)


SRC_DIR := ./src
BUILD_DIR := ./build

#SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')

OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
EXECUTABLE := just_launch_doom

.PHONY: all clean

all: $(BUILD_DIR) $(BUILD_DIR)/$(EXECUTABLE)

#assets_link:
#	mkdir -p $(BUILD_DIR)
#	ln -sfn $(PWD)/assets $(BUILD_DIR)/assets

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(EXECUTABLE): $(OBJECTS)


	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

mac_bundle: all
	mkdir -p $(BUILD_DIR)/JustLaunchDoom.app/Contents/MacOS
	mkdir -p $(BUILD_DIR)/JustLaunchDoom.app/Contents/Resources/assets
	cp $(BUILD_DIR)/${EXECUTABLE} $(BUILD_DIR)/JustLaunchDoom.app/Contents/MacOS/JustLaunchDoom
	cp build_assets/mac/Info.plist $(BUILD_DIR)/JustLaunchDoom.app/Contents/
	cp build_assets/mac/icon.icns $(BUILD_DIR)/JustLaunchDoom.app/Contents/Resources/icon.icns

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean all debug
