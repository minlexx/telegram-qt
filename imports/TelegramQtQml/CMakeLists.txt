project(TelegramQtQml)

add_definitions(-DTELEGRAMQT_QML_LIBRARY)

add_definitions(-DQT_DEPRECATED_WARNINGS)
add_definitions(-DQT_NO_CAST_FROM_ASCII)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC TRUE)

add_library(TelegramQt${QT_VERSION_MAJOR}Qml SHARED
    DeclarativeAuthOperation.cpp
    DeclarativeAuthOperation.hpp
    DeclarativeChatInfo.cpp
    DeclarativeChatInfo.hpp
    DeclarativeClient.cpp
    DeclarativeClient.hpp
    DeclarativeClientOperator.cpp
    DeclarativeClientOperator.hpp
    DeclarativeMessageSender.cpp
    DeclarativeMessageSender.hpp
    DeclarativeOperation.cpp
    DeclarativeOperation.hpp
    DeclarativeRsaKey.cpp
    DeclarativeRsaKey.hpp
    DeclarativeSettings.cpp
    DeclarativeSettings.hpp
    DeclarativeUserInfo.cpp
    DeclarativeUserInfo.hpp
)

target_link_libraries(TelegramQt${QT_VERSION_MAJOR}Qml
    Qt5::Core
    Qt5::Qml
)

target_include_directories(TelegramQt${QT_VERSION_MAJOR}Qml
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${QT5_INCLUDES}
)

target_link_libraries(TelegramQt${QT_VERSION_MAJOR}Qml
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    TelegramQt${QT_VERSION_MAJOR}
)

set(TELEGRAM_QT_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/TelegramQt${QT_VERSION_MAJOR})
set(TELEGRAM_QT_LIB_DIR ${CMAKE_INSTALL_LIBDIR})

set_target_properties(
    TelegramQt${QT_VERSION_MAJOR}Qml PROPERTIES
    SOVERSION ${TELEGRAM_QT_ABI}
    VERSION ${TELEGRAM_QT_VERSION}
)

install(
    TARGETS TelegramQt${QT_VERSION_MAJOR}Qml
    LIBRARY DESTINATION ${TELEGRAM_QT_LIB_DIR}
    COMPONENT Library
)

add_library(TelegramQmlPlugin SHARED plugin.cpp)

target_link_libraries(TelegramQmlPlugin
    Qt5::Core
    Qt5::Qml
    TelegramQt${QT_VERSION_MAJOR}Qml
)

install(
    TARGETS TelegramQmlPlugin
    DESTINATION ${INSTALL_QML_IMPORT_DIR}/TelegramQt
)

install(
    FILES qmldir plugins.qmltypes
    DESTINATION ${INSTALL_QML_IMPORT_DIR}/TelegramQt
)

if (DEVELOPER_BUILD)
    message(STATUS "Hint: To update plugins.qmltypes run the following command:\n"
        "    LD_LIBRARY_PATH=${CMAKE_INSTALL_FULL_LIBDIR} "
        "${_qt5Core_install_prefix}/bin/qmlplugindump "
        "-noinstantiate -nonrelocatable -v TelegramQt 0.2 "
        "${CMAKE_INSTALL_FULL_LIBDIR}/qt5/qml "
        "> ${CMAKE_CURRENT_SOURCE_DIR}/plugins.qmltypes"
        "\n"
        ""
        )
endif()
