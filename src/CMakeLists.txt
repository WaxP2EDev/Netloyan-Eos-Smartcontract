project(cpuloan)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( cpuloan cpuloan cpuloan.cpp )
target_include_directories( cpuloan PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( cpuloan ${CMAKE_SOURCE_DIR}/../ricardian )