cmake_minimum_required(VERSION 3.10)

project(groundwork)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_subdirectory(src/components)
add_subdirectory(src/containers)
add_subdirectory(src/game)


add_executable(groundwork src/main.cpp)

set(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/vendored/include")
set(SDL3_LIB_DIR "${CMAKE_SOURCE_DIR}/vendored/lib")

include_directories(${SDL3_INCLUDE_DIR})

target_link_libraries(groundwork PRIVATE 
  ${SDL3_LIB_DIR}/libSDL3.dll.a
  components
  containers
  game
)

add_custom_command(TARGET groundwork POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL3_LIB_DIR}/SDL3.dll"
    $<TARGET_FILE_DIR:groundwork>)
