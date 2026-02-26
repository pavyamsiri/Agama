python = import('python').find_installation(pure: false)
python_dep = python.dependency()

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


python_sources = [
  '../interface/python/interface_python.cpp'
]

# Python extension
python.extension_module('_agama',
  python_sources,
  include_directories: inc_dirs,
  dependencies: [agama_dep, python_dep, numpy_dep],
  link_with: agama_shared,
  cpp_args: common_args + ['-DHAVE_PYTHON'],
  link_args: ldflags,
  install: true
)

python.install_sources(
    [
        './__init__.py',
        './schwarzlib.py',
        './nemofile.py',
        './pygama.py',
        './agamacolormaps.py',
    ],
    subdir: 'agama',
)


# Custom target for tests
test_script = find_program('../py/alltest.py', required: false)
if test_script.found()
  test('agama-tests', test_script,
    workdir: meson.current_source_dir() / 'py',
    depends: [agama_shared]
  )
endif

