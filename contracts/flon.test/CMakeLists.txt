add_contract(flon.test flon.test ${CMAKE_CURRENT_SOURCE_DIR}/src/testcontract.cpp)

target_include_directories(flon.test
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(flon.test
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/testcontract.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/testcontract.contracts.md @ONLY )

target_compile_options( flon.test PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
