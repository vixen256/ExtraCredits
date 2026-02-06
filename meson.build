project('ExtraCredits', 'c', 'cpp', 'nasm', version: '1.0.0')
warning_level = 3
cpp_std = 'c++20'
optimization = 3
b_lto = true
b_pgo = 'use'

cpp = meson.get_compiler('cpp')

add_project_arguments(
	cpp.get_supported_arguments(
		'-D_WIN32_WINNT=_WIN32_WINNT_WIN10',
		'-std=c++20',
		'-masm=intel',
		'-O3',
	),
	language: 'cpp',
)

add_project_link_arguments(
	cpp.get_supported_arguments(
		'-static',
		'-s',
		'-std=c++20',
		'-ld3dcompiler',
		'-O3',
	),
	language: 'cpp',
)

minhook = subproject('minhook')
tomlc17 = subproject('tomlc17')

library(
	'ExtraCredits',
	include_directories: [
		'src',
	],
	cpp_pch : 'src/helpers.h',
	dependencies : [
		minhook.get_variable('minhook_dep'),
		tomlc17.get_variable('tomlc17_dep'),
	],
	sources : [
		'src/mod.cpp',
		'src/diva.cpp',
		'src/SigScan.cpp',
	],
	name_prefix: ''
)
