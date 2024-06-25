set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

cmake_minimum_required(VERSION 3.10)

project(Stark VERSION 1.0)

set(STARK_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(STARK_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)


file(GLOB STARK_HEADERS 
	"${STARK_CODE_DIR}/*.h"
)

file(GLOB STARK_SRC
	"${STARK_CODE_DIR}/*.cpp"
)

add_executable(test_tokenizer ${STARK_TESTS_DIR}/test_tokenizer.cpp ${STARK_HEADERS} ${STARK_SRC})
target_include_directories(test_tokenizer PUBLIC ${STARK_CODE_DIR})

add_executable(test_parser ${STARK_TESTS_DIR}/test_basic_dom_parser.cpp ${STARK_HEADERS} ${STARK_SRC})
target_include_directories(test_parser PUBLIC ${STARK_CODE_DIR})