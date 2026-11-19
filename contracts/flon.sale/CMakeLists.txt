add_contract(flon.sale flon.sale ${CMAKE_CURRENT_SOURCE_DIR}/src/flon.sale.cpp)

target_include_directories(flon.sale
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(flon.sale
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
