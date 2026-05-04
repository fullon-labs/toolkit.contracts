add_contract(flon.applybp flon.applybp ${CMAKE_CURRENT_SOURCE_DIR}/src/flon.applybp.cpp)

target_include_directories(flon.applybp
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(flon.applybp
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/flon.applybp.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/flon.applybp.contracts.md @ONLY )

target_compile_options( flon.applybp PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
