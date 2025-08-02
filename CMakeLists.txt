cmake_minimum_required(VERSION 3.16)

project(MyRaylibGame VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# external
add_subdirectory(External/Submodules/raylib)
add_subdirectory(External/Submodules/kcp)

# local
file(GLOB_RECURSE TARGET_SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(my_game ${TARGET_SOURCE_FILES})
target_include_directories(my_game
	PRIVATE SYSTEM
		"External/Submodules/raylib/src"
		"External/Submodules/kcp"
)
target_link_libraries(my_game
	PRIVATE
		raylib
		kcp
)
