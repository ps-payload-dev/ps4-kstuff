#!/bin/sh
# Copyright (C) 2026 ItsJokerZz
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.

set -eu

UPSTREAM_URL=https://github.com/Scene-Collective/ps4-hen.git
UPSTREAM_DIR=kpayload/include
FILES="amd_helper.h ccp_helper.h elf_helper.h freebsd_helper.h pfs_helper.h rif_helper.h sbl_helper.h sections.h self_helper.h sparse.h"

cd "$(dirname "$0")/.."

ref=${1:-HEAD}
python=
for candidate in python3 python; do
    if "$candidate" -c "" >/dev/null 2>&1; then
        python=$candidate
        break
    fi
done
[ -n "$python" ] || { echo "sync_hen.sh: Python 3 not found" >&2; exit 1; }

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

git init -q "$tmp"
git -C "$tmp" fetch -q --depth 1 "$UPSTREAM_URL" "$ref"
git -C "$tmp" -c core.autocrlf=false checkout -q FETCH_HEAD
commit=$(git -C "$tmp" rev-parse HEAD)

stage="$tmp/stage"
mkdir -p "$stage/include" "$stage/source/generated"

for f in $FILES; do
    git -C "$tmp" show "$commit:$UPSTREAM_DIR/$f" > "$stage/include/$f"
done

"$python" tools/generate_tables.py "$tmp" "$stage"

mkdir -p vendor/ps4-hen
for f in $FILES; do
    cp "$stage/include/$f" "vendor/ps4-hen/$f"
done

cp "$stage/source/generated/firmware_table.inc" source/generated/firmware_table.inc
cp "$stage/source/generated/remoteplay_table.inc" source/generated/remoteplay_table.inc
cp "$stage/source/generated/exploit_fixes_table.inc" source/generated/exploit_fixes_table.inc

printf '%s\n%s\n' "$UPSTREAM_URL" "$commit" > vendor/ps4-hen/UPSTREAM
echo "synced with $commit"
