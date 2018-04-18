[![Build Status](https://travis-ci.org/JulienMasson/jwm.svg?branch=master)](https://travis-ci.org/JulienMasson/jwm)

JWM
===

A minimalist floating/tiling WM, written over the XCB library.
Initially it was based on 2bwm.

Features
========

* Move / Resize / Split windows
* Change focus
* Hide / Raise windows
* Multi-screen support
* Fullscreen on one or all monitors
* Display wallpaper
* Toggle panel
* Reload conf
* Start programs

![Screenshot](https://raw.githubusercontent.com/JulienMasson/jwm/master/res/screenshot.png)

Compilation
============

jwm depends on these libraries:
+ x11-xcb
+ xcb-randr
+ xcb-keysyms
+ xcb-icccm
+ xcb-ewmh
+ xcb-util
+ cairo
+ pangocairo

Please install these libraries:

    $ sudo apt-get install libx11-xcb-dev libxcb-randr0-dev libxcb-keysyms1-dev libxcb-icccm4-dev libxcb-ewmh-dev libxcb-util-dev libcairo2-dev libpangocairo-1.0-0

Build jwm:

    $ git clone https://github.com/JulienMasson/jwm
    $ cd jwm
    $ make

Run it !
========

Please use [.xinitrc](https://github.com/JulienMasson/jwm/tree/master/res/.xinitrc) as an example.
This file has to be in your home folder.

Start X session on any tty:

    $ startx

### Usage

Run help command:

    $ jwm --help

```
NAME
       jwm - A minimalist floating WM, written over the XCB

SYNOPSIS
       jwm [OPTION...]

OPTIONS
       -c, --conf
              Specifies which configuration file to use instead of the default.

       -h, --help
              Display this help and exits.
```
### Key bindings

Mod key is referred to "windows" key.

| Key               | Action                                     |
|:------------------|:-------------------------------------------|
| `Mod-Right`       | Focus on next window                       |
| `Mod-Left`        | Focus on previous window                   |
| `Mod-Ctrl, Right` | Split window on vertical right             |
| `Mod-Ctrl, Left`  | Split window on vertical left              |
| `Mod-c`           | Delete focus window                        |
| `Mod-f`           | Fullscreen the focus window on one monitor |
| `Mod-t`           | Fullscreen the focus window on all monitor |
| `Mod-h`           | Hide focus window                          |
| `Mod-a`           | Raise all hidden windows                   |
| `Mod-r`           | Reload conf                                |
| `Mod-p`           | Toggle panel                               |
| `Mod-d`           | Start rofi                                 |
| `Mod-Return`      | Start urxvt                                |
| `Mod-Shift, e`    | Start emacs                                |
| `Mod, i`          | Volume up                                  |
| `Mod, u`          | Volume down                                |
| `Mod, o`          | Toggle volume                              |
| `Mod, p`          | Toggle panel                               |
| `Mod, l`          | Lock screen                                |
| `Mod-Shift, q`    | Exit jwm                                   |

External software
=================

Some keys are bind to external software:
+ rofi
+ urxvt
+ emacs
+ amixer
+ pactl
+ i3lock-fancy

To use it, please install these packages:

    $ sudo apt-get install rofi rxvt-unicode emacs alsa-utils i3lock-fancy

Configuration
=============

You can configure some settings in your [.jwmrc](https://github.com/JulienMasson/jwm/tree/master/res/.jwmrc)

| Option      | Action                                                          |
|:------------|:----------------------------------------------------------------|
| `log_level` | Log level: 0 (NO LOG), 1 (ERROR), 2 (WARN), 3 (INFO), 4 (DEBUG) |
| `log_file`  | Path to the log file                                            |
| `wallpaper` | Path to the wallpaper                                           |

Coding Style
============

Use the script in tools folder:

    $ ./tools/check_coding_style.sh
