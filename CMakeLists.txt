cmake_minimum_required(VERSION 3.10)

project(groundwork)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(src/components)
add_subdirectory(src/containers)
add_subdirectory(src/game)

set(Vulkan_INCLUDE_DIR "$ENV{VK_PATH}/Include")
set(Vulkan_LIBRARY "$ENV{VK_PATH}/Lib/vulkan-1.lib")
find_package(Vulkan REQUIRED)

add_executable(groundwork src/main.cpp)

set(SDL3_INCLUDE_DIR "$ENV{SDL_PATH}/include")
set(SDL3_LIB_DIR "$ENV{SDL_PATH}/lib")
find_package(SDL3 REQUIRED)



target_link_libraries(groundwork PRIVATE 
  SDL3::SDL3
  Vulkan::Vulkan
  components
  containers
  game
)

