cmake_minimum_required(VERSION 3.15)

project(rpn-calculator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(ENABLE_DOCTESTS "Enable doctests" OFF)

find_package(Fmt REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if (ENABLE_DOCTESTS)
  include(Doctest)
endif()

set(TARGET_BUILD_FLAGS
    "$<$<CXX_COMPILER_ID:MSVC>:/W4;/WX;/O2>"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wconversion;-Wformat=2;-Wunused;-Werror;-O3>")

file(GLOB APP_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_PROJECT_NAME}_v??.cpp")

foreach(source ${APP_SOURCES})
  get_filename_component(target_name ${source} NAME_WLE)
  add_executable(${target_name} ${source})
  target_link_libraries(${target_name} PRIVATE fmt::fmt)
  target_compile_options(${target_name} PRIVATE ${TARGET_BUILD_FLAGS})
  target_include_directories(${target_name} PRIVATE ${FMT_INCLUDE_DIR})

  if (ENABLE_DOCTESTS)
    target_compile_definitions(${target_name} PRIVATE ENABLE_DOCTESTS)
    target_include_directories(${target_name} PRIVATE ${DOCTEST_INCLUDE_DIR})
    target_link_libraries(${target_name} PRIVATE doctest)
  endif()
endforeach()
