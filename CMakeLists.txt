cmake_minimum_required(VERSION 3.16)

project(Labeling VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

qt_standard_project_setup()

qt_add_executable(appLabeling
    config.cpp
    config.h
    control.cpp
    control.h
    imagedisplay.cpp
    imagedisplay.h
    main.cpp
)

qt_add_qml_module(appLabeling
    URI Labeling
    VERSION 1.0
    QML_FILES
        Main.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appLabeling PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appLabeling
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appLabeling
    PRIVATE
        Qt6::Quick
        ${OpenCV_LIBS}
)

include(GNUInstallDirs)
install(TARGETS appLabeling
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
