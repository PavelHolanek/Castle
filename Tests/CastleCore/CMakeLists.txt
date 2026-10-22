# Builds the plain C++ core (Source/CastleCore) without Unreal and runs its unit tests.
#
#   cmake -S Tests/CastleCore -B Tests/CastleCore/build
#   cmake --build Tests/CastleCore/build --config Debug
#   ctest --test-dir Tests/CastleCore/build -C Debug --output-on-failure

cmake_minimum_required(VERSION 3.24)
project(CastleCoreTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(googletest
	URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
# Link against the same C runtime as the tests instead of the static one.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(CASTLE_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/CastleCore")
file(GLOB CASTLE_CORE_SOURCES CONFIGURE_DEPENDS "${CASTLE_CORE_DIR}/Private/*.cpp")
# The module boilerplate is the only file that needs the engine.
list(FILTER CASTLE_CORE_SOURCES EXCLUDE REGEX "CastleCoreModule\\.cpp$")

add_library(CastleCore STATIC ${CASTLE_CORE_SOURCES})
target_include_directories(CastleCore PUBLIC "${CASTLE_CORE_DIR}/Public")
if(MSVC)
	# Same warning level as UnrealBuildTool, and no RTTI as in Unreal modules.
	target_compile_options(CastleCore PRIVATE /W4 /WX /GR- /permissive-)
else()
	target_compile_options(CastleCore PRIVATE -Wall -Wextra -Wshadow -Werror -fno-rtti)
endif()

add_executable(CastleCoreTests
	JunctionTests.cpp
	WallCurveTests.cpp
	WallLimitsTests.cpp
	WallNetworkTests.cpp
	WallOutlineTests.cpp)
target_link_libraries(CastleCoreTests PRIVATE CastleCore GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(CastleCoreTests)
