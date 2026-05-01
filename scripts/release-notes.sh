#!/usr/bin/env bash
# Generate a categorized markdown release-notes block from git log.
#
# Reads commits matching <range> (default = since the most recent
# annotated v* tag) and groups them by Conventional Commits prefix.
# Output is paste-ready for GitHub's release form or
# `gh release create --notes-file -`.
#
# Usage:
#   ./scripts/release-notes.sh                  # last v*..HEAD
#   ./scripts/release-notes.sh v1.0.0..v1.1.0   # explicit range
#   ./scripts/release-notes.sh v1.1.0..HEAD     # since v1.1.0
#
# Notes:
# - Categorisation reads only the subject line (`type(scope): subject`).
# - A commit whose body contains `BREAKING CHANGE:` also appears under
#   "Breaking Changes" in addition to its primary section.
# - Commits without a recognised prefix go to "Other Changes".

set -euo pipefail

cd "$(dirname "$0")/.."

# Resolve range.
if [[ $# -ge 1 ]]; then
    RANGE="$1"
else
    PREV="$(git tag --list 'v*' --sort=-v:refname | head -1)"
    if [[ -z "$PREV" ]]; then
        echo "error: no v* tags found and no range given" >&2
        echo "usage: $0 [<git-range>]" >&2
        exit 1
    fi
    RANGE="$PREV..HEAD"
fi

declare -A SECTION_TITLE=(
    [breaking]='💥 Breaking Changes'
    [feat]='✨ Features'
    [fix]='🐛 Bug Fixes'
    [perf]='⚡ Performance'
    [docs]='📝 Documentation'
    [build]='🛠️ Build / Toolchain'
    [refactor]='♻️ Refactoring'
    [test]='✅ Tests'
    [chore]='🧹 Maintenance'
    [other]='🔀 Other Changes'
)
ORDER=(breaking feat fix perf docs build refactor test chore other)

declare -A LINES   # bucket -> accumulated bullets

# Map a commit subject to a bucket. Returns the bucket name on stdout.
classify() {
    local subject="$1"
    case "$subject" in
        feat\(*\):*|feat:*)            echo feat ;;
        fix\(*\):*|fix:*)              echo fix ;;
        perf\(*\):*|perf:*)            echo perf ;;
        docs\(*\):*|docs:*)            echo docs ;;
        build\(*\):*|build:*)          echo build ;;
        refactor\(*\):*|refactor:*)    echo refactor ;;
        test\(*\):*|test:*)            echo test ;;
        chore\(*\):*|chore:*)          echo chore ;;
        # Pre-Conventional-Commits style: m<N>(scope): subject. Each
        # milestone shipped user-visible work, so file under Features.
        m[0-9]*\(*\):*)                echo feat ;;
        # release: bumps and similar were used pre-v1.1 — treat as chore.
        release:*)                     echo chore ;;
        *)                             echo other ;;
    esac
}

append() {
    local bucket="$1" line="$2"
    LINES[$bucket]="${LINES[$bucket]:-}${line}"$'\n'
}

# Pass 1: categorise by subject.
while IFS=' ' read -r sha subject; do
    [[ -z "$sha" ]] && continue
    bucket=$(classify "$subject")
    short=$(git rev-parse --short=7 "$sha")
    append "$bucket" "* ${subject} (\`${short}\`)"
done < <(git log --format='%H %s' --no-merges --reverse "$RANGE")

# Pass 2: surface BREAKING CHANGE: commits in the breaking bucket too.
while read -r sha; do
    [[ -z "$sha" ]] && continue
    if git log -1 --format='%B' "$sha" | grep -q '^BREAKING CHANGE:'; then
        subject=$(git log -1 --format='%s' "$sha")
        short=$(git rev-parse --short=7 "$sha")
        append breaking "* ${subject} (\`${short}\`)"
    fi
done < <(git log --format='%H' --no-merges "$RANGE")

# Emit.
echo "## What's changed"
echo
emitted_any=0
for bucket in "${ORDER[@]}"; do
    content="${LINES[$bucket]:-}"
    [[ -z "$content" ]] && continue
    emitted_any=1
    echo "### ${SECTION_TITLE[$bucket]}"
    echo
    printf '%s' "$content"
    echo
done

if [[ "$emitted_any" -eq 0 ]]; then
    echo "_No commits in range._"
    echo
fi

# Footer.
LO="${RANGE%..*}"
HI="${RANGE#*..}"
[[ "$HI" == "HEAD" ]] && HI="$(git rev-parse --short HEAD)"
echo "**Full diff:** \`${LO}...${HI}\`"
