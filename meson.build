project('autoignore', 'cpp',
  version : '1.0.0',
  license : 'Apache-2.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++20',
    'buildtype=release'
  ]
)

# Compiler setup
cpp = meson.get_compiler('cpp')

# Check for required dependencies
filesystem_dep = cpp.find_library('stdc++fs', required : false)
if not filesystem_dep.found()
  filesystem_dep = dependency('', required : false)
endif

# Source files
sources = files(
  'src/main.cpp',
  'src/template_mix.cpp'
)

# Main executable
autoignore_exe = executable('autoignore',
  sources,
  dependencies : [filesystem_dep],
  install : true,
  install_dir : get_option('bindir')
)

# Install templates
template_dir = get_option('datadir') / 'autoignore' / 'template'
install_subdir('template', 
  install_dir : get_option('datadir') / 'autoignore',
  strip_directory : false
)

# Summary
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