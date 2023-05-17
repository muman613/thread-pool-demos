project(foolibs)

include_directories(inc ../common)
add_library(fooutils STATIC src/fooUtils.cpp inc/fooUtils.h)
add_library(foosocketman STATIC src/fooSocketMan.cpp inc/fooSocketMan.h)
add_library(foovariantmap STATIC src/fooVariantMap.cpp inc/fooVariantMap.h)
add_library(foosimpletp STATIC src/fooSimpleThreadPool.cpp inc/fooSimpleThreadPool.h)
add_library(foocomplxtp src/fooComplexThreadPool.cpp inc/fooComplexThreadPool.h)
