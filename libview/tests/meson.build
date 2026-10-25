cflags = [
  '-DG_LOG_DOMAIN="PapersViewTests"',
  '-DPAPERS_COMPILATION',
  '-DTESTDATADIR="@0@"'.format(join_paths(source_root, 'test-data'))
]

test_executable = executable(
  'test-pps-jobs',
  'test-pps-jobs.c',
  include_directories: [top_inc],
  dependencies: libppsview_dep,
  c_args: cflags
)

test('pps-jobs-test', test_executable, env: backend_env)

annotation_test = executable(
  'test-pps-annotations',
  'test-pps-annotations.c',
  gnome.compile_resources(
    'annotation-test-resources',
    'annotations.gresource.xml',
    source_dir: data_dir,
    c_name: 'annotation_test'
  ),
  include_directories: [top_inc, context_inc],
  dependencies: libppsview_dep,
  c_args: cflags
)

test('pps-annotations-test', annotation_test, env: backend_env)

annotation_no_icons_test = executable(
  'test-pps-annotations-no-icons',
  'test-pps-annotations.c',
  include_directories: [top_inc, context_inc],
  dependencies: libppsview_dep,
  c_args: cflags + ['-DTEST_NO_CURSOR_RESOURCES']
)

test('pps-annotations-no-icons-test', annotation_no_icons_test, env: backend_env)
