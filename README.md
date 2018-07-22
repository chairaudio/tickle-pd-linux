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

# build

$ ninja -C build

# install

$ ninja -C build install
