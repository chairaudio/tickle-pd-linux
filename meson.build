project('tickle-pd', 'cpp',
  default_options : [
    'cpp_std=c++17',
    'default_library=static',
    'install_dir=/home/@0@/Documents/Pd/externals'.format(run_command('whoami').stdout().strip())
  ]
)

stdcppfs_dep = meson.get_compiler('cpp').find_library('stdc++fs')

install_dir = get_option('install_dir')
if install_dir.startswith('~')
  install_dir = install_dir.split('~')[1]
  install_dir = '/home/@0@@1@'.format(run_command('whoami').stdout().strip(), install_dir)
endif


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
    stdcppfs_dep
  ]
)

tickle_pd_helpfile_dep = dependency('tickle_pd_helpfile', fallback : ['tickle-pd-helpfile', 'tickle_pd_helpfile_dep'])
meson.add_install_script('scripts/copy-helpfile.py', install_dir)



