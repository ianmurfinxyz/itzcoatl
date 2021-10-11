
set(CMAKE_CXX_FLAGS -Wall)

set(PXR_SOURCE
        src/pxr_bmp.cpp
        src/pxr_collision.cpp
        src/pxr_engine.cpp
        src/pxr_gfx.cpp
        src/pxr_hud.cpp
        src/pxr_input.cpp
        src/pxr_log.cpp
        src/pxr_particle.cpp
        src/pxr_rand.cpp
        src/pxr_rc.cpp
        src/pxr_sfx.cpp
        src/pxr_wav.cpp
        src/pxr_xml.cpp
        src/tinyxml2.cpp)

add_library(pixiretro ${PXR_SOURCE})
target_include_directories(pixiretro PUBLIC include)
target_link_libraries(pixiretro -lSDL2 -lSDL2_mixer -lSDL2 -lm -lGLX_mesa)