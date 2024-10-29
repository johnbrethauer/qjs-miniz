project('layout', 'c')

quickjs = dependency('quickjs', static:true)
miniz = dependency('miniz', static:true)

libtype = get_option('default_library')
if libtype != 'static'
  add_project_arguments('-DJS_SHARED_LIBRARY', language:'c')
endif

inc = include_directories('.')

miniz_lib = library('miniz',
  'qjs_miniz.c',
  include_directories:inc,
  dependencies: [quickjs, miniz]
)

qjs_miniz_dep = declare_dependency(
  link_with: miniz_lib,
  include_directories:inc,  
)
