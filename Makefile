CXX := clang++
WIN_CXX := x86_64-w64-mingw32-g++

SRC_DIR := ./src
SRC_DIR_WIN := src
BUILD_DIR := ./build
BUILD_ASSETS_DIR := ./build_assets
DEBUGFLAGS := -g -O0

SDL_DIR_WIN := C:/libs/SDL2-2.30.2/x86_64-w64-mingw32
SDL_LIB_DIR_WIN := $(SDL_DIR_WIN)/lib
SDL_INCLUDE_DIR := $(SDL_DIR_WIN)/include

$(info SDL_LIB_DIR_WIN:)
$(info $(SDL_LIB_DIR_WIN))

WIN_CXXFLAGS := -std=c++17 -I$(SDL_INCLUDE_DIR)/SDL2 $(DEBUGFLAGS) -DSDL_MAIN_HANDLED -DSDL_STATIC

#CXXFLAGS := -std=c++17 $(shell sdl2-config --cflags) -I/usr/local/include -I./ $(DEBUGFLAGS)
#SDL_STATIC_LIBS_ARM := $(shell sdl2-config --static-libs)
#SDL_STATIC_LIBS_X86 := $(shell /usr/local/bin/sdl2-config --static-libs)
#LDFLAGS_ARM := $(SDL_STATIC_LIBS_ARM)
#LDFLAGS_X86 := $(SDL_STATIC_LIBS_X86)
#WIN_SDL2_LIB_FILEPATH := C:\libs\SDL2-2.30.2\x86_64-w64-mingw32\lib\libSDL2.dll.a
#WIN_LDFLAGS := -L$(SDL_LIB_DIR_WIN) -static -lmingw32 -lSDL2main -lSDL2 -static-libgcc -static-libstdc++ -mwindows
WIN_LDFLAGS := -L$(SDL_LIB_DIR_WIN) -static -lmingw32 -lSDL2main -lSDL2 -mwindows -lwinmm -limm32 -lversion -lole32 -loleaut32 -lsetupapi -static-libgcc -static-libstdc++


$(info LDFLAGS:)
$(info $(WIN_LDFLAGS))

RC := windres
ICON := $(BUILD_ASSETS_DIR)/win/app.rc
ICON_OBJ := $(BUILD_DIR)/app.res

ifeq ($(OS),Windows_NT)
SOURCES := $(shell powershell.exe -Command "Get-ChildItem -Recurse -File -Filter *.cpp . | Resolve-Path -Relative | ForEach-Object { $$_ -replace '\\', '/' }")
else
SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
endif

OBJECTS_ARM64 :=  $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/arm64/%.o,$(SOURCES))
OBJECTS_X86_64 := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/x86_64/%.o,$(SOURCES))
OBJECTS_WIN :=    $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/win/%.o,$(SOURCES))

EXECUTABLE := just_launch_doom

.PHONY: all clean mac windows

all: mac

$(BUILD_DIR)/win/%.o: $(SRC_DIR)/%.cpp
	@powershell -Command "New-Item -Path $(dir $@) -ItemType Directory -Force | Out-Null"
	$(WIN_CXX) $(WIN_CXXFLAGS) -c $< -o $@ $(BUILD_ASSETS_DIR)/win/app.res

$(ICON_OBJ): $(ICON)
	@powershell -Command "New-Item -Path $(dir $@) -ItemType Directory -Force | Out-Null"
	$(RC) -I$(SRC_DIR) -O coff -o $@ $<

$(BUILD_DIR)/arm64/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -target arm64-apple-macos10.15

$(BUILD_DIR)/x86_64/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -target x86_64-apple-macos10.15

$(BUILD_DIR)/win/%.o: $(SRC_DIR)/%.cpp
	@powershell -Command "New-Item -Path $(dir $@) -ItemType Directory -Force | Out-Null"
	$(WIN_CXX) $(WIN_CXXFLAGS) -c $< -o $@

windows: $(OBJECTS_WIN) $(ICON_OBJ)
	$(WIN_CXX) -o $(BUILD_DIR)/$(EXECUTABLE).exe $^ $(WIN_LDFLAGS)

$(BUILD_DIR)/$(EXECUTABLE)_arm64: $(OBJECTS_ARM64)
	$(CXX) -arch arm64 $(LDFLAGS_ARM) $^ -o $@

$(BUILD_DIR)/$(EXECUTABLE).exe: $(OBJECTS_WIN)
	$(WIN_CXX) $(LDFLAGS_WIN) $^ -o $@

$(BUILD_DIR)/$(EXECUTABLE)_x86_64: $(OBJECTS_X86_64)
	$(CXX) -arch x86_64 $(LDFLAGS_X86) $^ -o $@

$(BUILD_DIR)/$(EXECUTABLE)_universal: $(BUILD_DIR)/$(EXECUTABLE)_arm64 $(BUILD_DIR)/$(EXECUTABLE)_x86_64
	lipo -create -output $@ $^

mac: $(BUILD_DIR)/$(EXECUTABLE)_universal
	mkdir -p $(BUILD_DIR)/JustLaunchDoom.app/Contents/MacOS
	mkdir -p $(BUILD_DIR)/JustLaunchDoom.app/Contents/Resources
	cp $(BUILD_DIR)/$(EXECUTABLE)_universal $(BUILD_DIR)/JustLaunchDoom.app/Contents/MacOS/JustLaunchDoom
	cp $(BUILD_ASSETS_DIR)/mac/Info.plist $(BUILD_DIR)/JustLaunchDoom.app/Contents/
	cp $(BUILD_ASSETS_DIR)/mac/icon.icns $(BUILD_DIR)/JustLaunchDoom.app/Contents/Resources/

clean:
	$(RM_BUILD)

ifeq ($(OS),Windows_NT)
RM_BUILD = cmd /c "if exist build rmdir /s /q build"
MKDIR = mkdir
else
RM_BUILD = rm -rf build
MKDIR = mkdir -p
endif
