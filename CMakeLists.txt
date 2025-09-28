cmake_minimum_required(VERSION 3.25)
project(bundleme C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/dirent)

add_compile_options(-Wall -Wextra -pedantic)

add_executable(bundleme
    main.c
)
