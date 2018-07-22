# prerequesites

python3, fmt, meson, ninja

# configure

$ meson build

## set installation directory option at setup-time

$ meson build -Dinstall_dir=/my/install/dir

## set installation directory option later

$ meson configure build -Dinstall_dir=/my/install/dir

# build

$ ninja -C build

# install

$ ninja -C build install
