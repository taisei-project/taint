#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

if ! git remote | grep -qP '^upstream-dawn$'; then
    git remote add -t main --no-tags upstream-dawn https://dawn.googlesource.com/dawn
fi

git fetch --no-tags upstream-dawn
REV="$(git rev-parse upstream-dawn/main)"

git subtree split --prefix=src/tint  upstream-dawn/main -b upstream-tint
git subtree split --prefix=src/utils upstream-dawn/main -b upstream-utils

git subtree merge --prefix=src/tint  upstream-tint  --squash -m "Pull src/tint from dawn @ $REV"
git subtree merge --prefix=src/utils upstream-utils --squash -m "Pull src/utils from dawn @ $REV"
