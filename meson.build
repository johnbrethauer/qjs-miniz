project('layout', 'c')

quickjs = dependency('quickjs', static:true)
miniz = dependency('miniz', static:true)

inc = include_directories('.')

miniz_lib = library('miniz',
  'qjs_miniz.c',
  include_directories:inc,
  dependencies: [quickjs, miniz],
  c_shared_args: '-DJS_SHARED_LIBRARY',
)

qjs_miniz_dep = declare_dependency(
  link_with: miniz_lib,
  include_directories:inc,  
)
