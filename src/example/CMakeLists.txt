add_executable(arbtest arbtest.cc)
target_link_libraries(arbtest ssl crypto ws utilws rest utilrest  ${Boost_LIBRARIES})

add_executable(arb arb.cc)
target_link_libraries(arb ssl crypto ws utilws rest utilrest  ${Boost_LIBRARIES})
