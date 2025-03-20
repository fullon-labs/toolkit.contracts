add_contract(tokensummary tokensummary ${CMAKE_CURRENT_SOURCE_DIR}/tokensummary.cpp)

target_include_directories(tokensummary
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/)

set_target_properties(tokensummary
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/tokensummary.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/tokensummary.contracts.md @ONLY )

target_compile_options( tokensummary PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
