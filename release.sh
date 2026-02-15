#!/bin/bash
set -e

# --- Configuration ---
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENV_FILE="$SCRIPT_DIR/.env"
BUILD_DIR="$SCRIPT_DIR/build"
DRY_RUN=false

# --- Helpers ---
info() { echo "==> $1"; }
error() { echo "ERROR: $1" >&2; exit 1; }

# --- Parse args ---
for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=true ;;
        v*) VERSION="$arg" ;;
        *) error "Unknown argument: $arg" ;;
    esac
done

if [ -z "$VERSION" ]; then
    echo "Usage: $0 <version> [--dry-run]"
    echo "Example: $0 v0.1.14"
    echo "         $0 v0.1.14 --dry-run  # build everything but skip tag push and GitHub release"
    exit 1
fi

if $DRY_RUN; then
    info "DRY RUN — will build and package but skip tagging and GitHub release"
fi

[ -f "$ENV_FILE" ] || error ".env file not found. Copy .env.example to .env and fill in your values."
source "$ENV_FILE"

[ -n "$WIN_HOST" ] || error "WIN_HOST not set in .env"
[ -n "$WIN_WORKSPACE" ] || error "WIN_WORKSPACE not set in .env"
[ -n "$WSL_HOST" ] || error "WSL_HOST not set in .env"
[ -n "$WSL_WORKSPACE" ] || error "WSL_WORKSPACE not set in .env"

if ! $DRY_RUN; then
    command -v gh >/dev/null 2>&1 || error "gh CLI not found. Install it: https://cli.github.com"

    BRANCH=$(git -C "$SCRIPT_DIR" rev-parse --abbrev-ref HEAD)
    [ "$BRANCH" = "main" ] || error "Must be on main branch (currently on $BRANCH)"

    if ! git -C "$SCRIPT_DIR" diff --quiet || ! git -C "$SCRIPT_DIR" diff --cached --quiet; then
        error "Working tree is not clean. Commit or stash your changes first."
    fi
fi

# --- Tag ---
if $DRY_RUN; then
    info "Skipping tag creation (dry run)"
else
    info "Creating and pushing tag $VERSION..."
    git -C "$SCRIPT_DIR" tag "$VERSION"
    git -C "$SCRIPT_DIR" push origin "$VERSION"
fi

# --- Mac build ---
info "Building Mac..."
make -C "$SCRIPT_DIR" clean
make -C "$SCRIPT_DIR" mac

info "Packaging Mac zip..."
(cd "$BUILD_DIR" && zip -r "JustLaunchDoom-${VERSION}-Mac.zip" "JustLaunchDoom.app")

# --- Linux build ---
info "Building Linux (SSH to $WSL_HOST)..."
ssh "$WSL_HOST" "cd '$WSL_WORKSPACE' && git pull && make clean && make linux"

info "Copying Linux binary..."
scp "$WSL_HOST:$WSL_WORKSPACE/build/just_launch_doom_linux" "$BUILD_DIR/just_launch_doom_linux"

info "Packaging Linux zip..."
(cd "$BUILD_DIR" && cp just_launch_doom_linux just_launch_doom && zip "JustLaunchDoom-${VERSION}-Linux.zip" just_launch_doom && rm just_launch_doom)

# --- Windows build ---
info "Building Windows (SSH to $WIN_HOST)..."
ssh "$WIN_HOST" "cd '$WIN_WORKSPACE' && git pull && make clean && make windows"

info "Copying Windows binary..."
scp "$WIN_HOST:$WIN_WORKSPACE/build/JustLaunchDoom.exe" "$BUILD_DIR/JustLaunchDoom.exe"

info "Packaging Windows zip..."
(cd "$BUILD_DIR" && zip "JustLaunchDoom-${VERSION}-Win.zip" JustLaunchDoom.exe)

# --- GitHub Release ---
info "All artifacts built:"
ls -lh "$BUILD_DIR"/JustLaunchDoom-${VERSION}-*.zip

if $DRY_RUN; then
    info "Dry run complete! Zips are in $BUILD_DIR. Run without --dry-run to create the GitHub release."
else
    info "Creating GitHub release $VERSION..."
    gh release create "$VERSION" \
        "$BUILD_DIR/JustLaunchDoom-${VERSION}-Mac.zip" \
        "$BUILD_DIR/JustLaunchDoom-${VERSION}-Linux.zip" \
        "$BUILD_DIR/JustLaunchDoom-${VERSION}-Win.zip" \
        --title "$VERSION" \
        --notes ""

    info "Done! Release $VERSION created: https://github.com/mtmckenna/just-launch-doom/releases/tag/$VERSION"
fi
