cmake_minimum_required(VERSION 2.8)

project(practice_server)

include_directories(./src ./src/core ./src/rtmp ./include)

set(SOURCE_FILES ./src/main.cpp)
aux_source_directory(./src SOURCE_FILES)
aux_source_directory(./src/core SOURCE_FILES)
aux_source_directory(./src/rtmp SOURCE_FILES)

link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(practice_server ${SOURCE_FILES})

