cmake_minimum_required(VERSION 3.19)
project(audioplayer LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Core5Compat Widgets Multimedia MultimediaWidgets)
if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
endif()

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_ICON_RESOURCE)
if(WIN32)
    list(APPEND APP_ICON_RESOURCE appIcon.rc)
endif()

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
    ${APP_ICON_RESOURCE}
)

target_link_libraries(audioplayer
    PRIVATE
        Qt6::Core
        Qt6::Core5Compat
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::MultimediaWidgets
)

function(enable_project_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

enable_project_warnings(audioplayer)

if(BUILD_TESTING)
    qt_add_executable(audioplayer_tests
        tests/lrcparser_test.cpp
        lrcparser.h
        lrcparser.cpp
        lyricscontroller.h
        lyricscontroller.cpp
        audioanalyzer.h
        audioanalyzer.cpp
    )
    target_include_directories(audioplayer_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(audioplayer_tests
        PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Multimedia Qt6::Test)
    enable_project_warnings(audioplayer_tests)
    add_test(NAME audioplayer_tests COMMAND audioplayer_tests)
endif()

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
