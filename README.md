This Pure Data external provides access to the Tickle instrument by The Center for Haptic Audio Interaction Research (CHAIR) inside Pure Data.

This git lives at https://gitlab.chair.audio/chair-audio/tickle-pd-linux (private)

and is pushed automatically to https://github.com/chairaudio/tickle-pd-linux

# prerequisites

meson

# configure

$ meson build

# optional: set custom installation directory
Default installation path is ~/pd-externals/
if you are fine with this go to build and install. 
alternatively configure a custom installation directory like this:

## set installation directory option at setup-time

$ meson build -Dinstall_dir=/my/install/dir

## set installation directory option later

$ meson configure build -Dinstall_dir=/my/install/dir

# a) build only

$ ninja -C build

# b) build and install

$ ninja -C build install
