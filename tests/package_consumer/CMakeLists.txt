cmake_minimum_required(VERSION 3.25)

project(hwm14_package_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(hwm14 CONFIG REQUIRED)

add_executable(hwm14_package_consumer
  main.cpp
)

target_link_libraries(hwm14_package_consumer PRIVATE hwm14::hwm14)
