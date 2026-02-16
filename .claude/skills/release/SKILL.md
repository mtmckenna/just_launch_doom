# Release Skill

## Description
Build and publish a release of Just Launch Doom for Mac, Linux, and Windows.

## Instructions

1. Confirm we're on `main` with a clean working tree:
   ```bash
   git status
   ```
   If not on `main` or tree is dirty, stop and tell the user.

2. Read the current version from `src/main.cpp` (look for `#define VERSION`).

3. Ask the user what the new version should be (suggest bumping the patch number).

4. Generate release notes:
   - Find the previous release tag: `gh release list --limit 1`
   - Get the commit log since that tag: `git log {previous_tag}..HEAD --oneline`
   - Write concise, user-facing release notes summarizing the changes. Group by category (e.g. "New features", "Bug fixes", "Improvements") if applicable. Skip internal/CI changes. Use markdown bullet points.
   - Show the draft notes to the user and ask for approval or edits.

5. Ask the user if they want a dry run first or go straight to release.

6. Run the release script:
   ```bash
   ./release.sh v{version} --notes "{approved notes}"
   ```
   Or for dry run:
   ```bash
   ./release.sh v{version} --dry-run
   ```

7. After a successful release, verify it was created:
   ```bash
   gh release view v{version}
   ```
   Share the release URL with the user.
