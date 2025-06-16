CXX := clang++
CXX_WIN := /mingw64/bin/g++.exe
SRC_DIR := ./src
SRC_DIR_WIN := src
BUILD_DIR := ./build
BUILD_ASSETS_DIR := ./build_assets
DEBUGFLAGS := -g -O0
SDL_CFLAGS := $(shell sdl2-config --cflags)
CXXFLAGS := $(DEBUGFLAGS) -std=c++17 $(SDL_CFLAGS) -I/usr/local/include -I./
CXXFLAGS_LINUX := -std=c++17 $(SDL_CFLAGS) -I./ $(DEBUGFLAGS)
CXXFLAGS_WIN := -std=c++17 $(SDL_CFLAGS) -I./ $(DEBUGFLAGS)
UNAME_S := $(shell uname -s)
EXECUTABLE := just_launch_doom

ifneq (,$(findstring Linux,$(UNAME_S)))
SDL_STATIC_LIBS_LINUX := $(shell sdl2-config --static-libs)
LDFLAGS_LINUX := $(SDL_STATIC_LIBS_LINUX)
endif

ifneq (,$(findstring Darwin,$(UNAME_S)))
SDL_STATIC_LIBS_ARM := $(shell sdl2-config --static-libs)
SDL_STATIC_LIBS_X86 := $(shell /usr/local/bin/sdl2-config --static-libs)
LDFLAGS_ARM := $(SDL_STATIC_LIBS_ARM)
LDFLAGS_X86 := $(SDL_STATIC_LIBS_X86)
endif

ifneq (,$(findstring MINGW,$(UNAME_S)))
SDL_LDFLAGS_WIN := $(shell sdl2-config --static-libs)
LDFLAGS_WIN := -static $(SDL_LDFLAGS_WIN) -mwin32
endif

# Windows icons
RC := windres
ICON := $(BUILD_ASSETS_DIR)/win/app.rc
ICON_OBJ := $(BUILD_DIR)/app.res

SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS_ARM64 :=  $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/arm64/%.o,$(SOURCES))
OBJECTS_X86_64 := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/x86_64/%.o,$(SOURCES))
OBJECTS_LINUX := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/linux/%.o,$(SOURCES))
OBJECTS_WIN :=    $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/win/%.o,$(SOURCES))

# Test targets
TEST_SOURCES := $(shell find tests -name '*.cpp')
TEST_OBJECTS := $(patsubst tests/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SOURCES))
# For each test file, find its corresponding implementation
TEST_IMPL_SOURCES := $(patsubst tests/%_test.cpp,src/%.cpp,$(TEST_SOURCES))
TEST_IMPL_OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_IMPL_SOURCES))

.PHONY: all clean mac windows linux test

all: mac windows linux

$(BUILD_DIR)/win/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX_WIN) $(CXXFLAGS_WIN) -c $< -o $@ $(BUILD_ASSETS_DIR)/win/app.res

$(ICON_OBJ): $(ICON)
	mkdir -p $(dir $@)
	$(RC) -I$(SRC_DIR) -O coff -o $@ $<

$(BUILD_DIR)/arm64/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -arch arm64 -mmacosx-version-min=13.0 -c $< -o $@

$(BUILD_DIR)/x86_64/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -arch x86_64 -mmacosx-version-min=13.0 -c $< -o $@ 

$(BUILD_DIR)/linux/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_LINUX) -c $< -o $@

$(BUILD_DIR)/win/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX_WIN) $(CXXFLAGS_WIN) -c $< -o $@

windows: $(OBJECTS_WIN) $(ICON_OBJ)
	$(CXX_WIN) -o $(BUILD_DIR)/JustLaunchDoom.exe $^ $(LDFLAGS_WIN)

$(BUILD_DIR)/$(EXECUTABLE)_arm64: $(OBJECTS_ARM64)
	$(CXX) -arch arm64 -mmacosx-version-min=13.0 $(LDFLAGS_ARM) $^ -o $@

$(BUILD_DIR)/$(EXECUTABLE).exe: $(OBJECTS_WIN)
	$(WIN_CXX) $(LDFLAGS_WIN) $^ -o $@

$(BUILD_DIR)/$(EXECUTABLE)_x86_64: $(OBJECTS_X86_64)
	$(CXX) -arch x86_64 -mmacosx-version-min=13.0 $(LDFLAGS_X86) $^ -o $@

$(BUILD_DIR)/$(EXECUTABLE)_linux: $(OBJECTS_LINUX)
	$(CXX) $(LDFLAGS_LINUX) $^ -o $@

$(BUILD_DIR)/$(EXECUTABLE)_universal: $(BUILD_DIR)/$(EXECUTABLE)_arm64 $(BUILD_DIR)/$(EXECUTABLE)_x86_64
	lipo -create -output $@ $^

linux: $(BUILD_DIR)/$(EXECUTABLE)_linux
	mkdir -p $(BUILD_DIR)
	$(BUILD_DIR)/$(EXECUTABLE)_linux

mac_dev: $(BUILD_DIR)/$(EXECUTABLE)_arm64
	$(BUILD_DIR)/$(EXECUTABLE)_arm64

mac: $(BUILD_DIR)/$(EXECUTABLE)_universal
	mkdir -p $(BUILD_DIR)/JustLaunchDoom.app/Contents/MacOS
	mkdir -p $(BUILD_DIR)/JustLaunchDoom.app/Contents/Resources
	cp $(BUILD_DIR)/$(EXECUTABLE)_universal $(BUILD_DIR)/JustLaunchDoom.app/Contents/MacOS/JustLaunchDoom
	cp $(BUILD_ASSETS_DIR)/mac/Info.plist $(BUILD_DIR)/JustLaunchDoom.app/Contents/
	cp $(BUILD_ASSETS_DIR)/mac/icon.icns $(BUILD_DIR)/JustLaunchDoom.app/Contents/Resources/

$(BUILD_DIR)/tests/%.o: tests/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_OBJECTS) $(TEST_IMPL_OBJECTS) $(BUILD_DIR)/tests/config_utils.o
	$(CXX) $(CXXFLAGS) $^ -o $(BUILD_DIR)/test_runner
	$(BUILD_DIR)/test_runner

clean:
	rm -rf build