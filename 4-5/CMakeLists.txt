project(4-5)

add_executable(4-5-server server.cpp)
target_link_libraries(4-5-server udpcommunication)
set_target_properties(4-5-server PROPERTIES OUTPUT_NAME server)

add_executable(4-5-gardener gardener.cpp)
target_link_libraries(4-5-gardener udpcommunication)
set_target_properties(4-5-gardener PROPERTIES OUTPUT_NAME gardener)

add_executable(4-5-flowerbed flowerbed.cpp)
target_link_libraries(4-5-flowerbed udpcommunication)
set_target_properties(4-5-flowerbed PROPERTIES OUTPUT_NAME flowerbed)
