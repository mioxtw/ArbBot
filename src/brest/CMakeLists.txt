file(GLOB SOURCES "*cc")
add_library(brest STATIC ${SOURCES})
target_include_directories(brest PUBLIC)
target_link_libraries(brest)
