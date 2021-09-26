file(GLOB SOURCES "*cc")
add_library(utilws STATIC ${SOURCES})
target_include_directories(utilws PUBLIC)
target_link_libraries(utilws ssl crypto)
