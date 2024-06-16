project(8)

add_executable(8-server server.cpp)
target_link_libraries(8-server udpcommunication)
set_target_properties(8-server PROPERTIES OUTPUT_NAME server)

add_executable(8-gardener gardener.cpp)
target_link_libraries(8-gardener udpcommunication)
set_target_properties(8-gardener PROPERTIES OUTPUT_NAME gardener)

add_executable(8-flowerbed flowerbed.cpp)
target_link_libraries(8-flowerbed udpcommunication)
set_target_properties(8-flowerbed PROPERTIES OUTPUT_NAME flowerbed)



add_executable(8-monitor monitor.cpp)
target_link_libraries(8-monitor udpcommunication)
set_target_properties(8-monitor PROPERTIES OUTPUT_NAME monitor)
