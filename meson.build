# Meson build script for Agama library
project('agama', 'cpp',
  version: '1.0',
  license: ['GPL', 'MIT', 'BSD'],
  default_options: [
    'cpp_std=c++20',
    'buildtype=release',
    'default_library=both',
    'warning_level=1'
  ]
)

# Help message
message('''
Installation script for the Agama library.

To build and install:
meson setup build
meson compile -C build
meson install -C build

The build recognizes some environment variables:
CXX will set the compiler;
CFLAGS will set additional compilation flags;
LDFLAGS will set additional link flags.

The build will attempt to find required and optional dependencies:
- GSL (required)
- Eigen (optional)
- UNSIO (optional)
- GLPK (optional)
''')

subdir('src')
subdir('py')
subdir('exe')


# Install various documentation files
install_data(
  ['INSTALL', 'LICENSE', 'NEWS', 'Makefile', 'Makefile.msvc', 'Makefile.list', 'Doxyfile'],
  install_dir: get_option('datadir') / 'agama'
)
install_subdir('tests', install_dir: get_option('datadir') / 'agama')
install_subdir('data', install_dir: get_option('datadir') / 'agama')
install_subdir('doc', install_dir: get_option('datadir') / 'agama')
# Install all type stubs
install_subdir(
  'stubs/agama',
  install_dir: python.get_install_dir() / 'agama-stubs',
  strip_directory: true
)
