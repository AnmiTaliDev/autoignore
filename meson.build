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

prefix_dir = get_option('prefix')
datadir_path = prefix_dir / get_option('datadir')
template_install_dir = datadir_path / 'autoignore' / 'template'

conf_data = configuration_data()
conf_data.set('VERSION', meson.project_version())
conf_data.set('DATADIR', datadir_path)
conf_data.set('TEMPLATE_DIR', template_install_dir)
conf_data.set('DEFAULT_OUTPUT', get_option('default_output'))
conf_data.set('MAX_DEPTH', get_option('max_detect_depth'))
conf_data.set('PAGE_SIZE', get_option('interactive_page_size'))

config_h = configure_file(
  input : 'include/Config.hpp.in',
  output : 'Config.hpp',
  configuration : conf_data
)

sources = files(
  'src/main.cpp',
  'src/TemplateStore.cpp',
  'src/Detector.cpp',
  'src/Interactive.cpp'
)

autoignore_exe = executable('autoignore',
  sources,
  config_h,
  include_directories : [include_directories('include'), include_directories('.')],
  dependencies : [filesystem_dep],
  install : true,
  install_dir : get_option('bindir')
)

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
  'prefix': prefix_dir,
  'bindir': get_option('bindir'), 
  'datadir': get_option('datadir'),
  'template_dir': template_install_dir,
}, section: 'Directories')

summary({
  'buildtype': get_option('buildtype'),
  'warning_level': get_option('warning_level'),
  'cpp_std': get_option('cpp_std'),
  'default_output': get_option('default_output'),
  'max_detect_depth': get_option('max_detect_depth'),
  'interactive_page_size': get_option('interactive_page_size'),
}, section: 'Build Options')
