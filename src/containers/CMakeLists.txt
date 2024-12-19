add_library(containers
    componentstorage.cpp
    registry.cpp
    sparseset.cpp
)
target_include_directories(containers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
