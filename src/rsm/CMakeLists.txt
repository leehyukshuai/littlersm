add_executable(littlersm main.cpp)

target_link_libraries(littlersm PRIVATE common)

target_compile_features(littlersm PUBLIC cxx_std_20)