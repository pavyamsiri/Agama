# Meson build script for Agama library
project('agama', 'cpp',
  version: '1.0',
  license: ['GPL', 'MIT', 'BSD'],
  default_options: [
    'cpp_std=c++11',
    'buildtype=release',
    'default_library=both',
    'warning_level=1'
  ]
)

# Help message
message('''
Installation script for the Agama library.

To build and install:
meson setup builddir
meson compile -C builddir
meson install -C builddir

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


# Import file system module
fs = import('fs')

# Basic configuration
root_dir = meson.current_source_dir()

# Determine compiler and basic flags
cpp = meson.get_compiler('cpp')
is_msvc = cpp.get_id() == 'msvc'
is_macos = build_machine.system() == 'darwin'
is_windows = build_machine.system() == 'windows'

# Process environment variables
cflags = []
if get_option('buildtype') != 'debug'
  cflags += is_msvc ? ['/O2', '/EHsc', '/nologo'] : ['-O2']
endif

# Add user-provided flags
env_cflags = run_command('sh', '-c', 'echo $CFLAGS', check: false).stdout().strip()
if env_cflags != ''
  cflags += env_cflags.split()
endif

ldflags = []
env_ldflags = run_command('sh', '-c', 'echo $LDFLAGS', check: false).stdout().strip()
if env_ldflags != ''
  ldflags += env_ldflags.split()
endif

# Architecture-specific flags
if is_macos
  arch = run_command('uname', '-m', check: true).stdout().strip()
  arch_flag = '-arch=' + arch
  if cpp.has_argument(arch_flag)
    cflags += arch_flag
    ldflags += arch_flag
  endif
elif not is_msvc and not is_macos
  if cpp.has_argument('-march=native')
    cflags += '-march=native'
  endif
endif

# OpenMP detection
omp_dep = dependency('openmp', required: false)
if omp_dep.found()
  message('Compiling with OpenMP support')
endif

# Python and NumPy dependencies
py_mod = import('python')
python = py_mod.find_installation('python3', required: true)
python_dep = python.dependency(embed: true)

# Try to find numpy
# Try Meson's numpy dependency first, fallback to manual detection
numpy_dep = dependency('numpy', required: false)
if not numpy_dep.found()
  numpy_inc = run_command(python, '-c', 
    'import numpy; print(numpy.get_include())', check: false)
  if numpy_inc.returncode() == 0
    numpy_inc_dir = numpy_inc.stdout().strip()
    numpy_dep = declare_dependency(include_directories: numpy_inc_dir)
  else
    error('NumPy not found - required for Python extension')
  endif
endif

# GSL dependency (required)
gsl_dep = dependency('gsl', required: true)

# Eigen dependency (optional)
eigen_dep = dependency('eigen3', required: true)

# UNSIO dependency (optional)
unsio_dep = dependency('unsio', required: false)

# GLPK dependency (optional)
glpk_dep = dependency('glpk', required: false)
if glpk_dep.found()
  glpk_args = ['-DHAVE_GLPK']
else
  glpk_args = []
endif

# Collect all compile arguments
common_args = cflags + glpk_args
if eigen_dep.found()
  common_args += ['-DHAVE_EIGEN']
endif
if unsio_dep.found()
  common_args += ['-DHAVE_UNSIO']
endif

# Source files
agama_sources = [
  'src/utils.cpp',
  'src/potential_ferrers.cpp',
  'src/math_sphharm.cpp',
  'src/interface_nemo.cpp',
  'src/df_spherical.cpp',
  'src/actions_factory.cpp',
  'src/interface_python.cpp',
  'src/galaxymodel_base.cpp',
  'src/actions_focal_distance_finder.cpp',
  'src/potential_king.cpp',
  'src/math_spline.cpp',
  'src/utils_config.cpp',
  'src/math_core.cpp',
  'src/galaxymodel_densitygrid.cpp',
  'src/actions_isochrone.cpp',
  'src/orbit.cpp',
  'src/potential_multipole.cpp',
  'src/math_fit.cpp',
  'src/galaxymodel_fokkerplanck.cpp',
  'src/actions_spherical.cpp',
  'src/orbit_variational.cpp',
  'src/potential_perfect_ellipsoid.cpp',
  'src/math_gausshermite.cpp',
  'src/galaxymodel_jeans.cpp',
  'src/actions_staeckel.cpp',
  'src/particles_io.cpp',
  'src/potential_spheroid.cpp',
  'src/math_geometry.cpp',
  'src/galaxymodel_losvd.cpp',
  'src/actions_torus.cpp',
  'src/potential_analytic.cpp',
  'src/potential_utils.cpp',
  'src/math_linalg.cpp',
  'src/galaxymodel_selfconsistent.cpp',
  'src/coord.cpp',
  'src/potential_base.cpp',
  'src/raga_binary.cpp',
  'src/math_ode.cpp',
  'src/galaxymodel_spherical.cpp',
  'src/cubature.cpp',
  'src/potential_composite.cpp',
  'src/raga_core.cpp',
  'src/math_optimization.cpp',
  'src/galaxymodel_velocitysampler.cpp',
  'src/df_base.cpp',
  'src/potential_cylspline.cpp',
  'src/raga_losscone.cpp',
  'src/math_random.cpp',
  'src/interface_amuse.cpp',
  'src/df_disk.cpp',
  'src/potential_dehnen.cpp',
  'src/raga_potential.cpp',
  'src/math_sample.cpp',
  'src/interface_c.cpp',
  'src/df_factory.cpp',
  'src/potential_disk.cpp',
  'src/raga_relaxation.cpp',
  'src/math_specfunc.cpp',
  'src/interface_fortran.cpp',
  'src/df_halo.cpp',
  'src/potential_factory.cpp',
  'src/raga_trajectory.cpp',
  'src/torus/Fit.cc',
  'src/torus/CHB.cc',
  'src/torus/Fit2.cc',
  'src/torus/GeneratingFunction.cc',
  'src/torus/Orb.cc',
  'src/torus/PJMNum.cc',
  'src/torus/Point_ClosedOrbitCheby.cc',
  'src/torus/Point_None.cc',
  'src/torus/Torus.cc',
  'src/torus/Toy_Isochrone.cc',
  'src/torus/WD_Numerics.cc',
]

python_sources = [
  'src/interface_python.cpp'
]

# Include directories
inc_dirs = include_directories('src', 'py')

# Dependencies
deps = [
  python_dep,
  numpy_dep,
  gsl_dep,
  omp_dep
]

if eigen_dep.found()
  deps += eigen_dep
endif

if unsio_dep.found()
  deps += unsio_dep
endif

if glpk_dep.found()
  deps += glpk_dep
endif

# Build shared library
agama_shared = shared_library('agama',
  agama_sources,
  include_directories: inc_dirs,
  dependencies: deps,
  cpp_args: common_args + ['-DHAVE_PYTHON'],
  link_args: ldflags,
  install: true,
  name_prefix: '',
  soversion: '1'
)

# Build static library (but don't install when building Python wheel)
agama_static = static_library('agama',
  agama_sources,
  include_directories: inc_dirs,
  dependencies: deps,
  cpp_args: common_args,
  link_args: ldflags,
  # install: not meson.is_subproject()  # Only install if not building as subproject (e.g., for Python wheel)
  install: false
)

# Python extension
python.extension_module('agama',
  python_sources,
  include_directories: inc_dirs,
  dependencies: deps,
  link_with: agama_shared,
  cpp_args: common_args + ['-DHAVE_PYTHON'],
  link_args: ldflags,
  install: true
)

# Executables
executable_sources = {
  'example1': 'exe/example1.cpp',
  'example2': 'exe/example2.cpp',
}

foreach name, source : executable_sources
  if fs.exists(source)
    executable(name,
      source,
      include_directories: inc_dirs,
      dependencies: deps,
      link_with: agama_shared,
      cpp_args: common_args,
      link_args: ldflags,
      install: true
    )
  endif
endforeach

# Install data files
install_subdir('data', install_dir: get_option('datadir') / 'agama')
install_subdir('doc', install_dir: get_option('datadir') / 'agama')
install_subdir('py', install_dir: get_option('datadir') / 'agama')
install_subdir('tests', install_dir: get_option('datadir') / 'agama')
# Install all type stubs
install_subdir(
  'stubs/agama',
  install_dir: py.get_install_dir() / 'your_package'-stubs / 'your_package',
  strip_directory: true
)

# Install various documentation files
install_data(
  ['INSTALL', 'LICENSE', 'NEWS', 'Makefile', 'Makefile.msvc', 'Makefile.list', 'Doxyfile'],
  install_dir: get_option('datadir') / 'agama'
)

# Custom target for tests
test_script = find_program('py/alltest.py', required: false)
if test_script.found()
  test('agama-tests', test_script,
    workdir: meson.current_source_dir() / 'py',
    depends: [agama_shared]
  )
endif
