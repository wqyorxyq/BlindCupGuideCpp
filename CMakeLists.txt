cmake_minimum_required(VERSION 3.20)

project(BlindCupGuideCpp
    VERSION 0.1.0
    DESCRIPTION "Camera guidance prototype for helping a blind user grab a cup"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BCG_ENABLE_SAPI "Use Windows SAPI for speech output when available" ON)

find_package(OpenCV REQUIRED COMPONENTS core videoio highgui imgproc dnn)

add_executable(blind_cup_guide
    src/Application.cpp
    src/CameraSource.cpp
    src/DetectionTypes.cpp
    src/GuidanceEngine.cpp
    src/MockObjectDetector.cpp
    src/OpenCvDnnObjectDetector.cpp
    src/SpeechSynthesizer.cpp
    src/main.cpp)

target_include_directories(blind_cup_guide
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(blind_cup_guide
    PRIVATE
        ${OpenCV_LIBS})

if(WIN32 AND BCG_ENABLE_SAPI)
    target_compile_definitions(blind_cup_guide PRIVATE BCG_ENABLE_SAPI=1)
    target_link_libraries(blind_cup_guide PRIVATE ole32 sapi)
endif()

if(MSVC)
    target_compile_options(blind_cup_guide PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(blind_cup_guide PRIVATE -Wall -Wextra -Wpedantic)
endif()
