# Determine compiler and basic flags
cpp = meson.get_compiler('cpp')
is_msvc = cpp.get_id() == 'msvc'
is_macos = build_machine.system() == 'darwin'
is_windows = build_machine.system() == 'windows'

cflags = []
ldflags = []
if get_option('buildtype') != 'debug'
  cflags += is_msvc ? ['/O2', '/EHsc', '/nologo'] : ['-O2']
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

deps = []
common_args = cflags
# Required dependencies
# GSL dependency (required)
gsl_dep = dependency('gsl', required: true)
deps += gsl_dep

# Optional dependencies
# OpenMP dependency (option)
omp_dep = dependency('openmp', required: false)
if omp_dep.found()
  message('Compiling with OpenMP support')
  deps += omp_dep
endif
# Eigen dependency (optional)
eigen_dep = dependency('eigen3', required: false)
eigen_args = []
if false and eigen_dep.found()
  message('Compiling with eigen3 support')
  common_args += ['-DHAVE_EIGEN']
  deps += eigen_dep
endif
# UNSIO dependency (optional)
unsio_dep = dependency('unsio', required: false)
unsio_args = []
if unsio_dep.found()
  message('Compiling with unsio support')
  common_args += ['-DHAVE_UNSIO']
  deps += unsio_dep
endif
# GLPK dependency (optional)
glpk_dep = dependency('glpk', required: false)
glpk_args = []
if glpk_dep.found()
  message('Compiling with glpk support')
  common_args += ['-DHAVE_GLPK']
  deps += glpk_dep
endif


# Collect all compile arguments
message(common_args)

inc_dirs = include_directories('./', './torus')



agama_sources = [
  'utils.cpp',
  'potential_ferrers.cpp',
  'math_sphharm.cpp',
  'interface_nemo.cpp',
  'df_spherical.cpp',
  'actions_factory.cpp',
  'galaxymodel_base.cpp',
  'actions_focal_distance_finder.cpp',
  'potential_king.cpp',
  'math_spline.cpp',
  'utils_config.cpp',
  'math_core.cpp',
  'galaxymodel_densitygrid.cpp',
  'actions_isochrone.cpp',
  'orbit.cpp',
  'potential_multipole.cpp',
  'math_fit.cpp',
  'galaxymodel_fokkerplanck.cpp',
  'actions_spherical.cpp',
  'orbit_variational.cpp',
  'potential_perfect_ellipsoid.cpp',
  'math_gausshermite.cpp',
  'galaxymodel_jeans.cpp',
  'actions_staeckel.cpp',
  'particles_io.cpp',
  'potential_spheroid.cpp',
  'math_geometry.cpp',
  'galaxymodel_losvd.cpp',
  'actions_torus.cpp',
  'potential_analytic.cpp',
  'potential_utils.cpp',
  'math_linalg.cpp',
  'galaxymodel_selfconsistent.cpp',
  'coord.cpp',
  'potential_base.cpp',
  'raga_binary.cpp',
  'math_ode.cpp',
  'galaxymodel_spherical.cpp',
  'cubature.cpp',
  'potential_composite.cpp',
  'raga_core.cpp',
  'math_optimization.cpp',
  'galaxymodel_velocitysampler.cpp',
  'df_base.cpp',
  'potential_cylspline.cpp',
  'raga_losscone.cpp',
  'math_random.cpp',
  'interface_amuse.cpp',
  'df_disk.cpp',
  'potential_dehnen.cpp',
  'raga_potential.cpp',
  'math_sample.cpp',
  'interface_c.cpp',
  'df_factory.cpp',
  'potential_disk.cpp',
  'raga_relaxation.cpp',
  'math_specfunc.cpp',
  'interface_fortran.cpp',
  'df_halo.cpp',
  'potential_factory.cpp',
  'raga_trajectory.cpp',
  'torus/Fit.cc',
  'torus/CHB.cc',
  'torus/Fit2.cc',
  'torus/GeneratingFunction.cc',
  'torus/Orb.cc',
  'torus/PJMNum.cc',
  'torus/Point_ClosedOrbitCheby.cc',
  'torus/Point_None.cc',
  'torus/Torus.cc',
  'torus/Toy_Isochrone.cc',
  'torus/WD_Numerics.cc',
]


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

agama_dep = declare_dependency(link_with: agama_shared, include_directories: inc_dirs)

