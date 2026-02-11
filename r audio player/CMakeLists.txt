cmake_minimum_required(VERSION 3.21)

project(r_audio_player
    VERSION 1.0.6
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(BUILD_SHARED_LIBS OFF)
set(APP_ID r_audio_player)
set(APP_NAME "r audio player")

find_package(Qt6 REQUIRED
    COMPONENTS
        Widgets
        Network
)

if (WIN32)
    find_package(TagLib REQUIRED)
else()
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(TAGLIB REQUIRED
        taglib
    )
endif()

add_executable(r_audio_player
    miniaudio_implementation.cpp
    settings.h
    main.cpp
    audioplayer.h
    audioplayer.cpp
    clicklabel.h
    clickslider.h
    folderdialog.h
    folderdialog.cpp
    library.h
    library.cpp
    mainwindow.h
    mainwindow.cpp
    settingsdialog.h
    settingsdialog.cpp
    resources.qrc
)

if (WIN32)
    add_definitions(
        -DUNICODE
        -D_UNICODE
    )
endif()

if (WIN32)
    target_sources(r_audio_player PRIVATE
        app.rc
    )

    set_target_properties(r_audio_player PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

target_compile_definitions(r_audio_player PRIVATE
    NOMINMAX
    QT_NO_KEYWORDS
    APP_NAME="${APP_NAME}"
)

target_link_libraries(r_audio_player PRIVATE
    Qt6::Widgets
    Qt6::Network
)

if (WIN32)
    target_link_libraries(r_audio_player PRIVATE
        TagLib::TagLib
    )
else()
    target_link_libraries(r_audio_player PRIVATE
        ${TAGLIB_LIBRARIES}
    )

    target_include_directories(r_audio_player PRIVATE
        ${TAGLIB_INCLUDE_DIRS}
    )

    target_compile_options(r_audio_player PRIVATE
        ${TAGLIB_CFLAGS_OTHER}
    )
endif()

if (WIN32)
    set_target_properties(r_audio_player PROPERTIES
        OUTPUT_NAME "${APP_NAME}"
    )
else()
    set_target_properties(r_audio_player PROPERTIES
        OUTPUT_NAME "${APP_ID}"
    )
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/r_audio_player.desktop.in
    ${CMAKE_BINARY_DIR}/${APP_ID}.desktop
    @ONLY
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