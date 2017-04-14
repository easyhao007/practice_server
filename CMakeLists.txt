cmake_minimum_required(VERSION 2.8)

project(practice_server)

include_directories(./src ./include)
aux_source_directory(./src SOURCE_FILES)
link_directories(${PROJECT_SOURCE_DIR}/lib)
add_executable(practice_server ${SOURCE_FILES})

