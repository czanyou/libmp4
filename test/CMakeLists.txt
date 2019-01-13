cmake_minimum_required (VERSION 2.8)

#project name
project(mp4_test)

#head file path
include_directories(
    ../src/base
    ../src/
)

#source directory
aux_source_directory(. DIR_SRCS)

message(${DIR_SRCS})

add_executable(mp4_test ${DIR_SRCS})

TARGET_LINK_LIBRARIES(mp4_test mp4 pthread m)
