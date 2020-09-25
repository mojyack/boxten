libboxten_headers = [
    'libboxten.hpp',
    'plugin.hpp',
    'type.hpp',
    'audiofile.hpp',
    'console.hpp',
    'playback.hpp',
    'playlist.hpp',
    'worker.hpp',
    'eventhook.hpp',
]

libboxten_sources = [
    'plugin.cpp',
    'audiofile.cpp',
    'console.cpp',
    'playback.cpp',
    'buffer.cpp',
    'playlist.cpp',
    'worker.cpp',
    'configuration.cpp',
    'module.cpp',
    'eventhook.cpp',
    'debug.cpp',
]

libboxten_include_dir = include_directories('.')

compiler = meson.get_compiler('cpp')
dl_dep = compiler.find_library('dl')

libboxten = shared_module(
    'boxten',
    libboxten_sources,
    dependencies: [dl_dep],
    include_directories: [libboxten_include_dir, config_include_dir],
    install : true)

install_headers(libboxten_headers, subdir : 'libboxten')

package_config = import('pkgconfig')
header = ['libboxten']
package_config.generate(libraries : libboxten,
             subdirs : header,
             version : '1.0',
             name : 'libboxten',
             filebase : 'libboxten',
             description : 'libboxten')