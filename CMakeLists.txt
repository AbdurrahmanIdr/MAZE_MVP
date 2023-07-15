cmake_minimum_required(VERSION 3.25)
project(maze_2d C)

set(CMAKE_C_STANDARD 11)

add_executable(maze_2d main.c)

# Add the SDL2 library
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

target_link_libraries(maze_2d ${SDL2_LIBRARIES} SDL2 SDL2_image)
