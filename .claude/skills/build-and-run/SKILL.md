---
name: build-and-run
description: Build and launch the app locally on macOS for manual testing. Use when you need to visually verify UI changes or test the app interactively.
disable-model-invocation: true
---

Build and run the app locally on macOS:

1. Run `make mac_dev` from the project root
2. This compiles for arm64 and immediately launches the app
3. The app window will open - the user can interact with it to verify changes
4. The process blocks until the user closes the app window
