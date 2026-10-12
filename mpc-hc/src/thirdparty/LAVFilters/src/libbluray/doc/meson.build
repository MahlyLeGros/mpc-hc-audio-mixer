# Copyright (C) 2025 Rob Hall
# SPDX-License-Identifier: MIT

if not get_option('enable_docs')
    subdir_done()
endif

doxygen = find_program('doxygen', required: true)

doxyfile = configure_file(
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: {'INPUT': meson.project_build_root() / 'src/libbluray',
                    'STRIP_FROM_PATH': meson.project_build_root() / 'src',
                    'OUTPUT_DIRECTORY': meson.current_build_dir(),
                    'PROJECT_VERSION': meson.project_version(),
                    'HTML_FOOTER': meson.current_source_dir() / 'doxygen-footer.html'})

custom_target('docs',
    depends: libbluray_internal_headers,
    env: ['QUIET=yes'],
    command: [doxygen, doxyfile],
    output: ['html'],
    install: true,
    install_tag: 'doc',
    install_dir: get_option('datadir') / 'doc/libbluray')
