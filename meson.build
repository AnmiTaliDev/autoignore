project('autoignore', 'cpp',
  version : '2.0.0',
  license : 'Apache-2.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++20',
    'buildtype=release'
  ]
)

cpp = meson.get_compiler('cpp')

filesystem_dep = cpp.find_library('stdc++fs', required : false)
if not filesystem_dep.found()
  filesystem_dep = dependency('', required : false)
endif

sources = files(
  'src/main.cpp',
  'src/TemplateStore.cpp',
  'src/Detector.cpp',
  'src/Interactive.cpp'
)

autoignore_exe = executable('autoignore',
  sources,
  include_directories : include_directories('include'),
  dependencies : [filesystem_dep],
  install : true,
  install_dir : get_option('bindir')
)

template_dir = get_option('datadir') / 'autoignore' / 'template'
install_subdir('template',
  install_dir : get_option('datadir') / 'autoignore',
  strip_directory : false
)

install_data('completion/autoignore.bash',
  install_dir : get_option('datadir') / 'bash-completion' / 'completions',
  rename : 'autoignore'
)
install_data('completion/_autoignore',
  install_dir : get_option('datadir') / 'zsh' / 'site-functions'
)
install_data('completion/autoignore.fish',
  install_dir : get_option('datadir') / 'fish' / 'vendor_completions.d'
)

summary({
  'prefix': get_option('prefix'),
  'bindir': get_option('bindir'), 
  'datadir': get_option('datadir'),
  'template_dir': template_dir,
}, section: 'Directories')

summary({
  'buildtype': get_option('buildtype'),
  'warning_level': get_option('warning_level'),
  'cpp_std': get_option('cpp_std'),
}, section: 'Build Options')