cmake_minimum_required (VERSION 2.8)

#project name
project(mp4)

#head file path
include_directories(
    base
)

#source directory
aux_source_directory(. DIR_SRCS)

message(${DIR_SRCS})

add_library(mp4 ${DIR_SRCS})
