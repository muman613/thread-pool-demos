cmake_minimum_required(VERSION 3.16.3)
project(thread-pool-demos)

set(CMAKE_CXX_STANDARD 17)

# Some of the samples use boost program options...
find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

include_directories("lib/inc")

# We need to link with thread library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_subdirectory(lib)
add_subdirectory(samples)
