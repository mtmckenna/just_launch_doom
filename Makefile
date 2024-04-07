CXX := clang++
CXXFLAGS := -std=c++17 $(shell sdl2-config --cflags) -I/usr/local/include -I./
SDL_STATIC_LIBS_ARM := $(shell sdl2-config --static-libs)
SDL_STATIC_LIBS_X86 := $(shell /usr/local/bin/sdl2-config --static-libs)
LDFLAGS_ARM := $(SDL_STATIC_LIBS_ARM)
LDFLAGS_X86 := $(SDL_STATIC_LIBS_X86)
DEBUGFLAGS := -g -O0
CXXFLAGS += $(DEBUGFLAGS)


SRC_DIR := ./src
BUILD_DIR := ./build

#SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')

OBJECTS_ARM64 := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/arm64/%.o,$(SOURCES))
OBJECTS_X86_64 := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/x86_64/%.o,$(SOURCES))

EXECUTABLE := just_launch_doom

.PHONY: all clean mac_bundle

all: mac_bundle

$(BUILD_DIR)/arm64/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -target arm64-apple-macos10.15

$(BUILD_DIR)/x86_64/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -target x86_64-apple-macos10.15

$(BUILD_DIR)/$(EXECUTABLE)_arm64: $(OBJECTS_ARM64)
	$(CXX) -arch arm64 $(LDFLAGS_ARM) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/$(EXECUTABLE)_x86_64: $(OBJECTS_X86_64)
	$(CXX) -arch x86_64 $(LDFLAGS_X86) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/$(EXECUTABLE)_universal: $(BUILD_DIR)/$(EXECUTABLE)_arm64 $(BUILD_DIR)/$(EXECUTABLE)_x86_64
	lipo -create -output $@ $^

mac_bundle: $(BUILD_DIR)/$(EXECUTABLE)_universal
	mkdir -p $(BUILD_DIR)/JustLaunchDoom.app/Contents/MacOS
	mkdir -p $(BUILD_DIR)/JustLaunchDoom.app/Contents/Resources
	cp $(BUILD_DIR)/$(EXECUTABLE)_universal $(BUILD_DIR)/JustLaunchDoom.app/Contents/MacOS/JustLaunchDoom
	cp build_assets/mac/Info.plist $(BUILD_DIR)/JustLaunchDoom.app/Contents/
	cp build_assets/mac/icon.icns $(BUILD_DIR)/JustLaunchDoom.app/Contents/Resources/

clean:
	rm -rf $(BUILD_DIR)
