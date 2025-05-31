cmake_minimum_required(VERSION 3.29)
project(SFML LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# SFML 라이브러리 가져오기
include(FetchContent)
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

# Boost 라이브러리 찾기
find_package(Boost REQUIRED COMPONENTS system thread)

# 소스 파일 추가
add_executable(${PROJECT_NAME} src/main.cpp
        src/GameData.hpp
        src/GameLogic.hpp
        src/GameLogic.cpp
        src/GameLoop.hpp
        src/GameLoop.cpp
        src/NetworkClient.hpp
        src/NetworkClient.cpp
        src/BoardRenderer.hpp
        src/BoardRenderer.cpp
        src/GameStateUpdater.hpp
        src/GameStateUpdater.cpp
        src/InputHandler.hpp
        src/InputHandler.cpp
)


# C++ 표준 설정
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# 헤더 파일 경로와 라이브러리 링크
if(Boost_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
endif()

# SFML 라이브러리 링크
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics)
