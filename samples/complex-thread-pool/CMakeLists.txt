project(complex-thread-pool)


find_package(CURL REQUIRED)

include_directories(${CURL_INCLUDE_DIR})
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} fooutils foocomplxtp ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
