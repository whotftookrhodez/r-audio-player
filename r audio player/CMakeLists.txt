cmake_minimum_required(VERSION 3.21)

project(r_audio_player
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(BUILD_SHARED_LIBS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(ZLIB REQUIRED)
find_package(TagLib REQUIRED)

add_executable(r_audio_player WIN32
    settings.h
    main.cpp
    audioplayer.h
    audioplayer.cpp
    folderdialog.h
    folderdialog.cpp
    library.h
    library.cpp
    mainwindow.h
    mainwindow.cpp
    settingsdialog.h
    settingsdialog.cpp
    app.rc
    resources.qrc
)

target_compile_definitions(r_audio_player PRIVATE
    NOMINMAX
    QT_NO_KEYWORDS
)

target_link_libraries(r_audio_player PRIVATE
    Qt6::Widgets
    TagLib::TagLib
)

set_target_properties(r_audio_player PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "r audio player"
)

if (WIN32)
    add_custom_command(TARGET r_audio_player POST_BUILD
        COMMAND "C:/Qt/6.10.1/msvc2022_64/bin/windeployqt.exe"
                --release
                --compiler-runtime
                --no-translations
                --no-system-d3d-compiler
                "$<TARGET_FILE:r_audio_player>"
    )
endif()