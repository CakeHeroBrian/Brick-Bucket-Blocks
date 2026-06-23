cmake_minimum_required(VERSION 3.24)
project(glad LANGUAGES C)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.c)

add_library(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    OUTPUT_NAME ${PROJECT_NAME}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)