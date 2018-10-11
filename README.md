This Pure Data external provides access to the Tickle instrument by The Center for Haptic Audio Interaction Research (CHAIR) inside Pure Data.

This git lives at https://gitlab.chair.audio/chair-audio/tickle-pd-linux (private)

and is pushed automatically to https://github.com/chairaudio/tickle-pd-linux

## prerequisites

`meson`

# submodules

The helpfile is a submodule. It needs to be pulled before building:
```
$ git submodule update --init
```
# configure

$ meson build

# optional: set custom installation directory
Default installation path is `~/pd-externals/`
if you are fine with this go to build and install. 
alternatively configure a custom installation directory like this:

## set installation directory option at setup-time
```
$ meson build -Dinstall_dir=/my/install/dir
```
## set installation directory option later
```
$ meson configure build -Dinstall_dir=/my/install/dir
```
# build
## a) build only
```
$ ninja -C build
``````
## b) build and install
```
$ ninja -C build install
```
# License and trademark

This software is copyright 2018 The Center for Haptic Audio Interaction Research (CHAIR).
It is licensed under a GPL-3 licence.

The CHAIR name and logo are registered trademarks and may not be used unauthorized.
