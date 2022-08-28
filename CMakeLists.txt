cmake_minimum_required(VERSION 3.7)
project(itzcoatl CXX)

set(GAME_SOURCE
	src/main.cpp
	src/menu_scene.cpp
	src/play_scene.cpp
	src/itzcoatl.cpp)

include(conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(itzcoatl ${GAME_SOURCE})
target_compile_features(itzcoatl PRIVATE cxx_std_17)
target_include_directories(itzcoatl PUBLIC include ${CONAN_INCLUDE_DIRS})
target_link_directories(itzcoatl PUBLIC ${CONAN_LIB_DIRS})
target_link_libraries(itzcoatl PUBLIC ${CONAN_LIBS})
