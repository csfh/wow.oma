#!/bin/sh
# Assemble dist/WoW.oma. Does not install it and does not start the game.
set -eu
cd "$(dirname "$0")"
mkdir -p build dist
cc -Wall -Wextra -Werror -O2 -o build/wow wow.c
cc -Wall -Wextra -Werror -O2 -o build/wowd wowd.c
rm -rf dist/WoW.oma
odk oma bundle build/wow --manifest manifest.toml --include build/wowd=bin/wowd -o dist
odk oma inspect dist/WoW.oma
