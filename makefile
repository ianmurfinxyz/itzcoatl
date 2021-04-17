LDLIBS = -lSDL2 -lSDL2_mixer -lm -lGLX_mesa
CXXFLAGS = -g -Wall -std=c++20

GSRC = source/main.cpp \
			 source/play_scene.cpp \
			 source/menu_scene.cpp \
			 source/snake.cpp

ESRC = source/pixiretro/pxr_bmp.cpp \
			 source/pixiretro/pxr_collision.cpp \
			 source/pixiretro/pxr_engine.cpp \
			 source/pixiretro/pxr_gfx.cpp \
			 source/pixiretro/pxr_hud.cpp \
			 source/pixiretro/pxr_input.cpp \
			 source/pixiretro/pxr_log.cpp \
			 source/pixiretro/pxr_particle.cpp \
			 source/pixiretro/pxr_rand.cpp \
			 source/pixiretro/pxr_rc.cpp \
			 source/pixiretro/pxr_sfx.cpp \
			 source/pixiretro/pxr_wav.cpp \
			 source/pixiretro/pxr_xml.cpp \
			 source/pixiretro/tinyxml2.cpp

GINC = source/play_scene.h \
			 source/menu_scene.h \
			 source/snake.h 

EINC = source/pixiretro/pxr_bmp.h \
			 source/pixiretro/pxr_collision.h \
			 source/pixiretro/pxr_color.h \
			 source/pixiretro/pxr_engine.h \
			 source/pixiretro/pxr_game.h \
			 source/pixiretro/pxr_gfx.h \
			 source/pixiretro/pxr_hud.h \
			 source/pixiretro/pxr_input.h \
			 source/pixiretro/pxr_log.h \
			 source/pixiretro/pxr_mathutil.h \
			 source/pixiretro/pxr_particle.h \
			 source/pixiretro/pxr_rand.h \
			 source/pixiretro/pxr_rc.h \
			 source/pixiretro/pxr_rect.h \
			 source/pixiretro/pxr_sfx.h \
			 source/pixiretro/pxr_vec.h \
			 source/pixiretro/pxr_wav.h \
			 source/pixiretro/pxr_xml.h \
			 source/pixiretro/tinyxml2.h

itzcoatl : $(GSRC) $(ESRC) $(GINC) $(EINC)
	$(CXX) $(CXXFLAGS) -o $@ $(GSRC) $(ESRC) $(LDLIBS) -I source/pixiretro/

.PHONY: clean
clean:
	rm itzcoatl *.o
