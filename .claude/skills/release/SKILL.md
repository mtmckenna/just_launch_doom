# Release Skill

## Description
Build and publish a release of Just Launch Doom for Mac, Linux, and Windows.

## Instructions

1. Confirm we're on `main` with a clean working tree:
   ```bash
   git status
   ```
   If not on `main` or tree is dirty, stop and tell the user.

2. Run tests:
   ```bash
   make test
   ```
   If any tests fail, stop and tell the user. Do not proceed with the release.

3. Determine the version:
   - Read the current version from `src/main.cpp` (look for `#define VERSION`).
   - Find the latest release tag: `gh release list --limit 1`
   - Compare the two. If they match, suggest bumping the patch number. If they differ, something may be off — tell the user what you found and ask them to clarify.
   - **Ask the user to confirm the version number before proceeding.** Do not continue until confirmed.

4. Generate release notes:
   - Get the commit log since the last release tag: `git log {previous_tag}..HEAD --oneline`
   - Write concise, user-facing release notes summarizing the changes. Group by category (e.g. "New features", "Bug fixes", "Improvements") if applicable. Skip internal/CI changes. Use markdown bullet points.
   - **Show the draft notes to the user and ask for approval or edits.** Do not continue until confirmed.

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
