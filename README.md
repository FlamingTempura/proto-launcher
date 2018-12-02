# Proto-Launcher

Minimalist and tiny (\~150KB) application launcher for Linux. Built in C++ with X11.

![image](https://user-images.githubusercontent.com/1085434/48985784-ad549e80-f103-11e8-9187-0261f25c6137.png)

![image](https://user-images.githubusercontent.com/1085434/49340913-078ebb80-f63e-11e8-9f92-41e7bfea697a.png)


Proto-launcher allows you to open applications which have desktop entries in the following directories:
* `/usr/share/applications`
* `/usr/local/share/applications`
* `~/.local/share/applications`

This has only been tested on Arch Linux -- comments and suggestions welcome on the issue tracker.

## Installation

```sh
git clone https://github.com/FlamingTempura/proto-launcher.git
cd proto-launcher
make install
```

You may also need to install libxft and ubuntu fonts.

To run the launcher:

```
proto-launcher
```

To use a keyboard combo to open the launcher, configure your desktop environment to run `proto-launcher` when you press a key shortcut.

## Color scheme and fonts

Use `F7` and `Shift`+`F7` to cycle through the included color schemes.

You can override colors and fonts in `~/.config/launcher.conf`. E.g.:

```
[Style]
title=#f8f8f2
comment=#75715e
match=#a6e22e
background=#272822
highlight=#49483E
regular=Ubuntu,sans-11
bold=Ubuntu,sans-11:bold
smallregular=Ubuntu,sans-10
smallbold=Ubuntu,sans-10:bold
large=Ubuntu,sans-20:light
```

![image](https://user-images.githubusercontent.com/1085434/49332368-0741e200-f5a4-11e8-8efb-4bfa71fbd3b9.png)

Colors must be 6-digit hexidecimal strings prefixed with a hash (e.g. `#ff0000`). Fonts must be written as `<families>-<size>:<options>` (e.g. `verdana-10:italic`). For more examples see the [fontconfig docs](https://www.freedesktop.org/software/fontconfig/fontconfig-user.html#AEN36).

## Scaling

Use `F6` and `Shift`+`F6` to adjust the scale of the launcher.

## Uninstall

```sh
make uninstall
```
