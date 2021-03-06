project(qml-client)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(app_SOURCES
    main.cpp
    models/DialogsModel.cpp
    models/DialogsModel.hpp
    models/ItemModelExtensions.hpp
    models/MessagesModel.cpp
    models/MessagesModel.hpp
    qml.qrc
)

find_package(Qt5 5.9 COMPONENTS Core Gui Quick QuickControls2 REQUIRED)

add_executable(qml-client-qt${QT_VERSION_MAJOR} ${app_SOURCES})

target_link_libraries(qml-client-qt${QT_VERSION_MAJOR} LINK_PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Quick
    Qt5::QuickControls2
)

target_link_libraries(qml-client-qt${QT_VERSION_MAJOR} LINK_PUBLIC
    TelegramQt${QT_VERSION_MAJOR}
    TelegramQt${QT_VERSION_MAJOR}Qml
)

install(
    TARGETS qml-client-qt${QT_VERSION_MAJOR}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "Hint: Add 'QML2_IMPORT_PATH=\"${CMAKE_INSTALL_FULL_LIBDIR}/qt5/qml\"' "
    "to 'qml-client-qt${QT_VERSION_MAJOR}' run enviroment variables to ensure that the QML import is correct.")
