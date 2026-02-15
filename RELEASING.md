# Releasing

## Prerequisites

- SSH access to the Linux (`WSL_HOST`) and Windows (`WIN_HOST`) build machines
- `gh` CLI installed and authenticated (`gh auth login`)
- On `main` branch with a clean working tree

## Setup

Copy `.env.example` to `.env` and fill in your build machine hostnames and workspace paths:

```bash
cp .env.example .env
```

## Running a release

```bash
./release.sh v0.1.15
```

To test the build pipeline without creating a tag or GitHub release:

```bash
./release.sh v0.1.15 --dry-run
```

This builds and packages all 3 zips but skips tagging, pushing, and the GitHub release. It also skips the clean-working-tree and main-branch checks so you can run it from any branch.

## What the script does

1. Validates `.env`, version argument, clean git state on `main`
2. Creates and pushes a git tag for the version
3. Builds the Mac universal binary locally, packages `JustLaunchDoom.app` into a zip
4. SSHes to the Linux machine, pulls latest, builds, copies the binary back, zips it
5. SSHes to the Windows machine, pulls latest, builds, copies the exe back, zips it
6. Creates a GitHub release with all 3 zip files attached
