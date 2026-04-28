cmake_minimum_required(VERSION 3.19)
project(audioplayer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)
set(KISSFFT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rd-party/kissfft")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


qt_add_executable(audioplayer
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    res.qrc
    audioanalyzer.h
    audioanalyzer.cpp
    spectrumwidget.h
    spectrumwidget.cpp
    lrcparser.h
    lrcparser.cpp
    lyricscontroller.h
    lyricscontroller.cpp
    karaokewidget.h
    karaokewidget.cpp
)

target_include_directories(audioplayer PRIVATE "${KISSFFT_ROOT}/include")

target_link_libraries(audioplayer
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::MultimediaWidgets
        "${KISSFFT_ROOT}/lib/kissfft-float.lib"
)

install(TARGETS audioplayer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET audioplayer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
