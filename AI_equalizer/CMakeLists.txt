cmake_minimum_required(VERSION 3.16)

project(AI_equalizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Multimedia Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Multimedia Network)

# Find PulseAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSEAUDIO REQUIRED libpulse-simple)

set(PROJECT_SOURCES
    src/main.cpp
    src/EqualizerMainWindow.cpp
    src/EqualizerMainWindow.h
    src/EqualizerMainWindow.ui
    src/equalizerengine.cpp
    src/equalizerengine.h
    src/audioprocessor.cpp
    src/audioprocessor.h
    src/PresetModel.cpp
    src/PresetModel.h
    src/ChatView.cpp
    src/ChatView.h
    src/EqualizerViewModel.cpp
    src/EqualizerViewModel.h
    src/AudioProcessingThread.cpp
    src/AudioProcessingThread.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AI_equalizer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(AI_equalizer
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(AI_equalizer PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Network
    ${PULSEAUDIO_LIBRARIES}
)

target_include_directories(AI_equalizer PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})

target_include_directories(AI_equalizer PRIVATE src)

set_target_properties(AI_equalizer PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AI_equalizer)
endif()
