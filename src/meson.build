prefix = get_option('prefix')
datadir = prefix / get_option('datadir') / 'boxten'

qt_dep = dependency('qt5', version: '>= 5.2', required: true, modules: ['Core', 'Widgets', 'Gui'])


config_data = configuration_data()
config_data.set('prefix', prefix)
config_data.set('datadir', datadir)
if get_option('buildtype').startswith('debug')
  config_data.set('DEBUG', true)
else
  config_data.set('DEBUG', false)
endif
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : config_data)
config_include_dir = include_directories('.')


subdir('libboxten')
subdir('boxten')