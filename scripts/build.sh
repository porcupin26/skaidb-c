#!/bin/bash
# Build libskaidb from the skaidb-rust repository at the version this
# checkout pins (SKAIDB_VERSION) and assemble a distribution tree:
#
#   scripts/build.sh [dist-dir]        # default ./dist
#
# Result: <dist>/include/skaidb.h, <dist>/include/skaidb.hpp,
#         <dist>/lib/libskaidb.a, <dist>/lib/libskaidb.so (or .dylib).
# Needs cargo (https://rustup.rs) and, for the `kerberos` feature
# (SKAIDB_FEATURES=kerberos), the MIT krb5 development headers.
set -euo pipefail
here=$(cd "$(dirname "$0")/.." && pwd)
dist="${1:-$here/dist}"
mkdir -p "$dist"
dist=$(cd "$dist" && pwd)   # absolute, portably (macOS realpath has no -m)
version=$(tr -d '[:space:]' < "$here/SKAIDB_VERSION")
src="${SKAIDB_RUST_SRC:-$here/.skaidb-rust}"
if [ ! -d "$src/.git" ]; then
  git clone -q --depth 1 --branch "v$version" https://github.com/porcupin26/skaidb-rust "$src"
else
  git -C "$src" fetch -q --depth 1 origin "refs/tags/v$version:refs/tags/v$version" && git -C "$src" checkout -q "v$version"
fi
features=${SKAIDB_FEATURES:-}
(cd "$src" && cargo build --release -p skaidb-ffi --locked ${features:+--features "$features"})
mkdir -p "$dist/include" "$dist/lib"
cp "$here/include/skaidb.h" "$here/include/skaidb.hpp" "$dist/include/"
cp "$src/target/release/libskaidb.a" "$dist/lib/"
for so in "$src"/target/release/libskaidb.so "$src"/target/release/libskaidb.dylib; do
  [ -f "$so" ] && cp "$so" "$dist/lib/"
done
printf '%s\n' "$version" > "$dist/VERSION"
echo "libskaidb $version -> $dist"
