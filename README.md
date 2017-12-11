jwm
===

A minimalist floating WM, written over the XCB library and based on 2bwm.

jwm does not come with any bar or panel.


Features:
=========

* Move / Resize windows
* Vertical split
* Multi-screen support
* Fullscreen


Compilation
============

Here are the dependencies:
+ x11-xcb
+ xcb-randr
+ xcb-keysyms
+ xcb-icccm
+ xcb-ewmh

Install these packages:

    $ sudo apt-get install libx11-xcb-dev libxcb-randr0-dev libxcb-keysyms1-dev libxcb-icccm4-dev libxcb-ewmh-dev

To build jwm:

    $ git clone https://github.com/JulienMasson/jwm
    $ cd jwm
    $ make

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

Coding Style
============

Use the script in tools folder:

    $ ./tools/check_coding_style.sh
