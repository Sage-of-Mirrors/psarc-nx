cmake_minimum_required(VERSION 3.12)
project(psarc-nx)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB PSARCNX_SRC
	# PSARC-NX
	"include/*.h"
	"include/*.hpp"
	"src/*.c"
	"src/*.cpp"
)

add_executable(psarc-nx ${PSARCNX_SRC})
target_include_directories(psarc-nx PUBLIC include)
