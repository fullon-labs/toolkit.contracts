add_contract(flon.airdrop flon.airdrop ${CMAKE_CURRENT_SOURCE_DIR}/src/flon.airdrop.cpp)

target_include_directories(flon.airdrop
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include )

set_target_properties(flon.airdrop
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/flon.airdrop.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/flon.airdrop.contracts.md @ONLY )

target_compile_options( flon.airdrop PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
