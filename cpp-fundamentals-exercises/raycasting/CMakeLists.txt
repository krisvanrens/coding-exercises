cmake_minimum_required(VERSION 3.15)

project(raycasting)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CURSES_NEED_WIDE True) # Use Ncursesw for Unicode support.

find_package(Curses REQUIRED)
find_package(Fmt REQUIRED)

set(TARGET_BUILD_FLAGS
    "$<$<CXX_COMPILER_ID:MSVC>:/W4;/WX;/O2>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wconversion;-Wformat=2;-Wunused;-Werror;-O3>")

file(GLOB APP_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_PROJECT_NAME}_v??.cpp")

foreach(source ${APP_SOURCES})
  get_filename_component(target_name ${source} NAME_WLE)
  add_executable(${target_name} ${source})
  target_link_libraries(${target_name} PRIVATE ${CURSES_LIBRARY} fmt::fmt)
  target_compile_options(${target_name} PRIVATE ${TARGET_BUILD_FLAGS})
  target_include_directories(${target_name} PRIVATE ${CURSES_INCLUDE_DIR} ${FMT_INCLUDE_DIR})
endforeach()
