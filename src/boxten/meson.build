thread_dep = dependency('threads')
boxten_dep = dependency('libboxten')

files = [
    'main.cpp',
    'config_tool.cpp',
]
executable('boxten',
    files,
    dependencies:[thread_dep, qt_dep, boxten_dep],
    include_directories : [libboxten_include_dir, config_include_dir],
    install : true)