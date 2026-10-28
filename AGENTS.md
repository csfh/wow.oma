# AGENTS.md

This repository builds an `.oma` bundle that launches World of Warcraft. The commands and the consent step are in [README.md](README.md). Contributions are covered by the [CLA](CLA.md).

`./test.sh` is safe to run. It never starts `wow.sh`. `./build.sh` assembles `dist/WoW.oma` and does not install it. `odk oma dev` installs the bundle and starts Battle.net on the user's session. Do not run it unless the user asks.
