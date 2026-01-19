cmake_minimum_required(VERSION 3.21)

project(r_audio_player
    VERSION 1.0.3
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(BUILD_SHARED_LIBS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(TagLib REQUIRED)

add_executable(r_audio_player WIN32
    settings.h
    main.cpp
    audioplayer.h
    audioplayer.cpp
    clickslider.h
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
    Qt6::Network
    TagLib::TagLib
)

set_target_properties(r_audio_player PROPERTIES
    OUTPUT_NAME "r audio player"
)

if (WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
        REQUIRED
    )

    add_custom_command(TARGET r_audio_player POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --release
                --compiler-runtime
                --no-translations
                --no-system-d3d-compiler
                "$<TARGET_FILE:r_audio_player>"
    )
endif()