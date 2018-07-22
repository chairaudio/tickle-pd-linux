project('tickle-pd', 'cpp',
  default_options : [
    'cpp_std=c++1z',
    'default_library=static',
    'install_dir=/home/@0@/pd-externals'.format(run_command('whoami').stdout().strip())
  ]
)

fmt_dep = dependency('fmt', fallback : ['fmt', 'fmt_dep'])
stdcppfs_dep = meson.get_compiler('cpp').find_library('stdc++fs',
  dirs : ['/usr/lib/gcc/x86_64-linux-gnu/7/']
)

install_dir = get_option('install_dir')

version_data = configuration_data()
version_data.set('version', run_command('git', 'describe').stdout().strip())
configure_file(input : './src/gitversion.h.in',
               output : 'gitversion.h',
               configuration : version_data)

shared_module('tickle~',
  'src/tickle.cc',
  'src/device.c++',
  'src/client.c++',
  'src/shared_device_manager.c++',
  install : true,
  install_dir : install_dir,
  name_suffix : 'pd_linux',
  name_prefix : '',
  dependencies : [
    fmt_dep, stdcppfs_dep
  ]
)

install_data(
  'data/tickle~-help.pd',
  install_dir : install_dir
)



