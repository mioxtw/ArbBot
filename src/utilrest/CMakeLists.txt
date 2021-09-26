file(GLOB SOURCES "*cc")
add_library(utilrest STATIC ${SOURCES})
target_include_directories(utilrest PUBLIC)
target_link_libraries(utilrest ssl crypto)
