# Release Skill

## Description
Guides the user through building and publishing a release of Just Launch Doom for Mac, Linux, and Windows.

## Instructions

1. Confirm we're on `main` with a clean working tree:
   ```bash
   git status
   ```

2. Read the current version from `src/main.cpp` (look for `#define VERSION`).

3. Ask the user what the new version should be (suggest bumping the patch number).

4. Run the release script:
   ```bash
   ./release.sh v{version}
   ```
   If the user wants to test first, suggest `--dry-run`:
   ```bash
   ./release.sh v{version} --dry-run
   ```

5. After the script completes, verify the GitHub release was created:
   ```bash
   gh release view v{version}
   ```
