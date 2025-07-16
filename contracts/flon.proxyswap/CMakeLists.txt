add_contract(flon.proxyswap flon.proxyswap ${CMAKE_CURRENT_SOURCE_DIR}/src/flon.proxyswap.cpp)

if(DEFINED ENV{DAY_SECONDS_FOR_TEST})
   message(WARNING "ENV{DAY_SECONDS_FOR_TEST}=$ENV{DAY_SECONDS_FOR_TEST} should use only for test!!!")
   target_compile_definitions(flon.proxyswap PUBLIC "DAY_SECONDS_FOR_TEST=$ENV{DAY_SECONDS_FOR_TEST}")
endif()


target_include_directories(flon.proxyswap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include

)

set_target_properties(flon.proxyswap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/proxyswap.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/proxyswap.contracts.md @ONLY )
target_compile_options( flon.proxyswap PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
