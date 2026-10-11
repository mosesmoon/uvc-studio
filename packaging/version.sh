# Sourced by the packaging scripts: reads ../VERSION (same rules as
# cmake/Version.cmake) and sets VNAME ("1.1.0", or "1.2.0 Beta 2"), VFILE
# ("v1.1.0", "v1.2.0-Beta2"), VNUM ("1.1.0.99", Windows resources) and VCODE
# (Android versionCode).
_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VER=$(sed -n 's/^VERSION=//p' "$_root/VERSION")
BETA=$(sed -n 's/^BETA=//p' "$_root/VERSION")
IFS=. read -r VMAJOR VMINOR VPATCH <<<"$VER"
VMINOR=${VMINOR:-0}; VPATCH=${VPATCH:-0}
if [ "${BETA:-0}" -gt 0 ]; then
  VNAME="$VER Beta $BETA"; VFILE="v$VER-Beta$BETA"; VBUILD=$BETA
else
  VNAME="$VER"; VFILE="v$VER"; VBUILD=99
fi
VNUM="$VMAJOR.$VMINOR.$VPATCH.$VBUILD"
# major*1000000 + minor*10000 + patch*100 + (Beta number, or 99 for the
# release): every Beta sorts before its release and after the one before.
# (Up to 1.0 Beta 17 the code was major*10000 + minor*100 + Beta: 10017.)
VCODE=$((VMAJOR * 1000000 + VMINOR * 10000 + VPATCH * 100 + VBUILD))
