# WoW

An `.oma` bundle that launches World of Warcraft by running `/home/hallas/Launchers/wow.sh`. That script starts Battle.net under gamescope and Proton. The bundle's daemon is the program that runs outside the sandbox, which is what can see the game prefix, Proton and gamescope.

`./build.sh` compiles the two programs, assembles `dist/WoW.oma` and inspects it. It needs `odk` on `PATH`. It does not install the bundle and it does not start the game.

`./test.sh` checks that the daemon starts its script once while that process is alive, and again after it exits. It does not run `wow.sh`.

`odk oma dev` in this directory builds the bundle, installs it over `~/Applications` and runs it. The first time, the consent dialog asks about the daemon. Agreeing lets this bundle run `wow.sh` as you, with your files and your session. A bundle with a daemon has to be installed. It cannot be run once.

## License

MIT. See [LICENSE](LICENSE). Copyright (c) 2026 Christoffer Hallas.

If you send a pull request or other contribution, you agree to the [CLA](CLA.md).
