thread_dep = dependency('threads')
boxten_dep = declare_dependency(link_with: libboxten, include_directories: libboxten_include_dir)

files = [
    'main.cpp',
    'config_tool.cpp',
]
executable('boxten',
    files,
    dependencies:[thread_dep, qt_dep, boxten_dep],
    include_directories : [libboxten_include_dir, config_include_dir],
    install : true)