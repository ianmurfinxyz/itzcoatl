project('pixiretro', 'cpp', default_options: ['cpp_std=c++17', 'warning_level=2'])

pxr_src = [
  'source/pixiretro/pxr_bmp.cpp',
  'source/pixiretro/pxr_engine.cpp',
  'source/pixiretro/pxr_rc.cpp',
  'source/pixiretro/pxr_gfx.cpp',
  'source/pixiretro/pxr_collision.cpp',
  'source/pixiretro/pxr_input.cpp',
  'source/pixiretro/pxr_sfx.cpp',
  'source/pixiretro/pxr_log.cpp',
  'source/pixiretro/pxr_particle.cpp',
  'source/pixiretro/pxr_wav.cpp',
  'source/pixiretro/pxr_xml.cpp',
  'source/pixiretro/pxr_rand.cpp',
  'source/pixiretro/pxr_hud.cpp',
  'source/pixiretro/lib/tinyxml2/tinyxml2.cpp',
  'source/play_scene.cpp',
  'source/snake.cpp',
  'source/main.cpp'
]

cc = meson.get_compiler('cpp')

projectdir = meson.current_source_dir()
lib_openal_dir = join_paths(projectdir, 'bin/lib/openal')
lib_openal = cc.find_library('openal', dirs: lib_openal_dir)
lib_m = cc.find_library('m', required: true)
lib_glx_mesa = cc.find_library('GLX_mesa', required: true)
lib_sdl2 = dependency('SDL2')

inc_pxr = include_directories('include', 'include/pixiretro')

executable('itzcoatl', 
           pxr_src, 
           dependencies: [lib_m, lib_glx_mesa, lib_sdl2, lib_openal], 
           include_directories: inc_pxr)
