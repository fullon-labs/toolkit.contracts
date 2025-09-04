add_contract(flon.creator flon.creator ${CMAKE_CURRENT_SOURCE_DIR}/src/flon.creator.cpp)

target_include_directories(flon.creator
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include )

set_target_properties(flon.creator
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/flon.creator.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/flon.creator.contracts.md @ONLY )

target_compile_options( flon.creator PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
