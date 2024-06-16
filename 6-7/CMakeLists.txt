project(6-7)

add_executable(6-7-server server.cpp)
target_link_libraries(6-7-server udpcommunication)
set_target_properties(6-7-server PROPERTIES OUTPUT_NAME server)

add_executable(6-7-gardener gardener.cpp)
target_link_libraries(6-7-gardener udpcommunication)
set_target_properties(6-7-gardener PROPERTIES OUTPUT_NAME gardener)

add_executable(6-7-flowerbed flowerbed.cpp)
target_link_libraries(6-7-flowerbed udpcommunication)
set_target_properties(6-7-flowerbed PROPERTIES OUTPUT_NAME flowerbed)



add_executable(6-7-monitor monitor.cpp)
target_link_libraries(6-7-monitor udpcommunication)
set_target_properties(6-7-monitor PROPERTIES OUTPUT_NAME monitor)
