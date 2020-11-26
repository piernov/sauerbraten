#!/bin/sh
cd $(dirname "$0")

SAUER_HOME="../.."

#ensure old package is gone
rm -f sauerbraten.dmg

#make the directory which our disk image will be made of
#use /tmp as a destination because copying ourself (xcode folder) whilst compiling causes the dog to chase its tail
SAUERPKG=`mktemp -d /tmp/sauerpkg.XXXXXX` || exit 1

#leave indicator of where temp directory is in case things break
ln -sf "$SAUERPKG" "build/sauerpkg"

#copy executable
CpMac -r "$SAUER_HOME/sauerbraten.app" "$SAUERPKG/Sauerbraten.app"
strip -u -r "$SAUERPKG/Sauerbraten.app/Contents/MacOS/sauerbraten_universal"

GAMEDIR="$SAUERPKG/Sauerbraten.app/Contents/Resources"

#copy readme and data and remove unneccesary stuff
CpMac -r "$SAUER_HOME/README.html" "$SAUERPKG/"
CpMac -r "$SAUER_HOME/docs" "$SAUERPKG/"
CpMac -r "$SAUER_HOME/data" "$GAMEDIR/"
CpMac -r "$SAUER_HOME/packages" "$GAMEDIR/"
CpMac -r "$SAUER_HOME/server-init.cfg" "$GAMEDIR/"
CpMac -r "$SAUER_HOME/src" "$SAUERPKG/src"
find -d "$SAUERPKG" -name ".svn" -exec rm -rf {} \\;
find "$SAUERPKG" -name ".DS_Store" -exec rm -f {} \\;
rm -rf "$SAUERPKG/src/xcode/build"

#finally make a disk image out of the stuff
echo creating dmg
hdiutil create -srcfolder "$SAUERPKG" -volname sauerbraten sauerbraten.dmg
hdiutil internet-enable -yes sauerbraten.dmg

#cleanup
rm -rf "$SAUERPKG"

