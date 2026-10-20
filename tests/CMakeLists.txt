find_package(Catch2 3 REQUIRED)

add_executable(test_gfq test_gfq.cpp)
target_link_libraries(test_gfq PRIVATE Catch2::Catch2WithMain channel_coding_lib)
target_include_directories(test_gfq PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME gfq COMMAND test_gfq)

add_executable(test_gf2_vector test_gf2_vector.cpp)
target_link_libraries(test_gf2_vector PRIVATE Catch2::Catch2WithMain channel_coding_lib)
target_include_directories(test_gf2_vector PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME gf2_vector COMMAND test_gf2_vector)

add_executable(test_gf2_matrix test_gf2_matrix.cpp)
target_link_libraries(test_gf2_matrix PRIVATE Catch2::Catch2WithMain channel_coding_lib)
target_include_directories(test_gf2_matrix PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME gf2_matrix COMMAND test_gf2_matrix)

add_executable(test_enc_gf2 test_enc_gf2.cpp)
target_link_libraries(test_enc_gf2 PRIVATE Catch2::Catch2WithMain channel_coding_lib)
target_include_directories(test_enc_gf2 PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME enc_gf2 COMMAND test_enc_gf2)

add_executable(test_gf4_ops test_gf4_ops.cpp)
target_link_libraries(test_gf4_ops PRIVATE Catch2::Catch2WithMain channel_coding_lib)
target_include_directories(test_gf4_ops PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME gf4_ops COMMAND test_gf4_ops)

add_executable(test_gf4_vector test_gf4_vector.cpp)
target_link_libraries(test_gf4_vector PRIVATE Catch2::Catch2WithMain channel_coding_lib)
target_include_directories(test_gf4_vector PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME gf4_vector COMMAND test_gf4_vector)