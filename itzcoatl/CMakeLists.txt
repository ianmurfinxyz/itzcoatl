set(GAME_SOURCE
        src/main.cpp
        src/menu_scene.cpp
        src/play_scene.cpp
        src/snake.cpp)

add_executable(itzcoatl ${GAME_SOURCE})
target_include_directories(itzcoatl PUBLIC include)
target_link_libraries(itzcoatl pixiretro)
