jwm
==========
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
+ xcb-randr
+ xcb-keysyms
+ xcb-icccm
+ xcb-ewmh

To build jwm:

    $ git clone https://github.com/JulienMasson/jwm
    $ cd jwm
    $ make
