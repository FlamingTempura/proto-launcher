# Proto-Launcher

Minimalist and tiny (<100KB) application launcher for Linux. Built in C++ with X11.

![screenshot from 2018-11-25 22-40-40-min](https://user-images.githubusercontent.com/1085434/48985784-ad549e80-f103-11e8-9187-0261f25c6137.png)

Proto-launcher allows you to open applications which have desktop entries in the following directories:
* `/usr/share/applications`
* `/usr/local/share/applications`
* `~/.local/share/applications`

This has only been tested on Arch Linux -- comments and suggestions welcome on the issue tracker.

## Installation

```sh
git clone https://github.com/FlamingTempura/proto-launcher.git
cd proto-launcher
make
make install
```

You may also need to install libxft.

## Running the launcher

To run the launcher:

```
proto-launcher
```

To use a keyboard combo to open the launcher, configure your desktop environment to run `proto-launcher` when you press a key shortcut.

## Uninstall

```sh
make uninstall
```
