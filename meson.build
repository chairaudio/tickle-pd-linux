project('tickle-pd', 'cpp',
  default_options : [
    'cpp_std=c++1z',
    'default_library=static'
  ]
)

fmt_dep = dependency('fmt', fallback : ['fmt', 'fmt_dep'])
stdcppfs_dep = meson.get_compiler('cpp').find_library('stdc++fs',
  dirs : ['/usr/lib/gcc/x86_64-linux-gnu/7/']
)

shared_module('tickle~',
  'src/tickle.cc',
  'src/device.c++',
  'src/client.c++',
  'src/shared_device_manager.c++',
  install : true,
  install_dir : '/home/@0@/Desktop/tickle-pd/bin'.format(run_command('whoami').stdout().strip()),
  name_suffix : 'pd_linux',
  name_prefix : '',
  dependencies : [
    fmt_dep, stdcppfs_dep
  ]
)
