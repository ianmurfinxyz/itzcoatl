LDLIBS = -lSDL2 -lSDL2_mixer -lm -lGLX_mesa
CXXFLAGS = -g -Wall -std=c++20
SRC = main.cpp \
			play_scene.cpp \
			menu_scene.cpp \
			snake.cpp \
			pxr_bmp.cpp \
			pxr_collision.cpp \
			pxr_engine.cpp \
			pxr_gfx.cpp \
			pxr_hud.cpp \
			pxr_input.cpp \
			pxr_log.cpp \
			pxr_particle.cpp \
			pxr_rand.cpp \
			pxr_rc.cpp \
			pxr_sfx.cpp \
			pxr_wav.cpp \
			pxr_xml.cpp \
			tinyxml2.cpp
INC = pxr_bmp.h pxr_collision.h pxr_color.h pxr_engine.h pxr_game.h pxr_gfx.h pxr_hud.h \
			pxr_input.h pxr_log.h pxr_mathutil.h pxr_particle.h pxr_rand.h pxr_rc.h pxr_rect.h \
			pxr_sfx.h pxr_vec.h pxr_wav.h pxr_xml.h tinyxml2.h

itzcoatl : $(SRC) $(INC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDLIBS)

.PHONY: clean
clean:
	rm si *.o
