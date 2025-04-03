#!/bin/bash

# Check if version number is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 v0.1.7"
    exit 1
fi

VERSION=$1
BUILD_DIR="build"

# Create Mac zip
echo "Creating Mac release..."
cd "$BUILD_DIR"
zip -r "JustLaunchDoom-${VERSION}-Mac.zip" "JustLaunchDoom.app"
cd ..

# Create Linux zip
echo "Creating Linux release..."
cd "$BUILD_DIR"
cp just_launch_doom_linux just_launch_doom
zip "JustLaunchDoom-${VERSION}-Linux.zip" just_launch_doom
rm just_launch_doom
cd ..

# Create Windows zip
echo "Creating Windows release..."
cd "$BUILD_DIR"
zip "JustLaunchDoom-${VERSION}-Win.zip" JustLaunchDoom.exe
cd ..

echo "Release packages created successfully!" 