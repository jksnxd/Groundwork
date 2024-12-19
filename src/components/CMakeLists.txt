add_library(components
    health.cpp
    position.cpp
)
target_include_directories(components PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
