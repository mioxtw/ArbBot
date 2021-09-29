file(GLOB SOURCES "*cc")
add_library(bws STATIC ${SOURCES})
target_include_directories(bws PUBLIC)
target_link_libraries(bws)
