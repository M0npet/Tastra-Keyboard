cmake_minimum_required(VERSION 3.25)

project(
    V3Keyboard
    VERSION 0.2.1
    DESCRIPTION "Privacy-first customizable virtual keyboard for Wayland"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Test WaylandClient)
find_package(Qt6GuiPrivate CONFIG REQUIRED)
find_package(Qt6WaylandClientPrivate CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)

set(
    INPUT_METHOD_V1_PROTOCOL
    "${WAYLAND_PROTOCOLS_DATADIR}/unstable/input-method/input-method-unstable-v1.xml"
)

add_library(v3keyboard-core STATIC
    src/core/keyboardcontroller.cpp
    src/core/keyboardmodel.cpp
    src/core/locallexicon.cpp
    src/core/typingengine.cpp
    src/core/clipboardhistory.cpp
    src/core/emojicatalog.cpp
)

target_include_directories(v3keyboard-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(v3keyboard-core
    PUBLIC
        Qt6::Core
)

add_library(v3keyboard-platform-kwin STATIC
    src/platform/kwin/kwininputmethodv1backend.cpp
    src/platform/kwin/kwininputmethodv1session.cpp
    src/platform/kwin/kwininputmethodv1connection.cpp
    src/platform/kwin/kwininputpanelintegration.cpp
    src/platform/kwin/kwininputpanelshellintegration.cpp
    src/platform/kwin/kwininputpanelsurface.cpp
)

qt_generate_wayland_protocol_client_sources(
    v3keyboard-platform-kwin
    PRIVATE_CODE
    NO_INCLUDE_CORE_ONLY
    FILES
        "${INPUT_METHOD_V1_PROTOCOL}"
)

target_include_directories(v3keyboard-platform-kwin
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(v3keyboard-platform-kwin
    PUBLIC
        v3keyboard-core
        Qt6::Core
        Qt6::WaylandClient
    PRIVATE
        Qt6::GuiPrivate
        Qt6::WaylandClientPrivate
)


add_library(v3keyboard-app STATIC
    src/app/keyboarduibridge.cpp
    src/app/keyboardlayoutmetrics.cpp
)

target_include_directories(v3keyboard-app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(v3keyboard-app
    PUBLIC
        v3keyboard-core
        Qt6::Core
        Qt6::Gui
)

qt_add_executable(v3-keyboard
    src/app/main.cpp
)

target_link_libraries(v3-keyboard
    PRIVATE
        v3keyboard-app
        v3keyboard-platform-kwin
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

set_source_files_properties(
    src/ui/Main.qml
    PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
)

set_source_files_properties(
    src/ui/icons/shift.svg
    PROPERTIES QT_RESOURCE_ALIAS icons/shift.svg
)
set_source_files_properties(
    src/ui/icons/shift-lock.svg
    PROPERTIES QT_RESOURCE_ALIAS icons/shift-lock.svg
)
set_source_files_properties(
    src/ui/icons/backspace.svg
    PROPERTIES QT_RESOURCE_ALIAS icons/backspace.svg
)
set_source_files_properties(
    src/ui/icons/enter.svg
    PROPERTIES QT_RESOURCE_ALIAS icons/enter.svg
)

qt_add_resources(
    v3-keyboard
    "v3keyboard-ui"
    PREFIX "/v3keyboard"
    FILES
        src/ui/Main.qml
        src/ui/icons/shift.svg
        src/ui/icons/shift-lock.svg
        src/ui/icons/backspace.svg
        src/ui/icons/enter.svg
)


if(BUILD_TESTING)
    add_executable(v3keyboard-smoke
        tests/smoke.cpp
    )

    target_link_libraries(v3keyboard-smoke
        PRIVATE
            Qt6::Core
            Qt6::Test
    )

    add_test(
        NAME v3keyboard-smoke
        COMMAND v3keyboard-smoke
    )

    add_executable(tst_keyboardcontroller
        tests/tst_keyboardcontroller.cpp
    )

    target_link_libraries(tst_keyboardcontroller
        PRIVATE
            v3keyboard-core
            Qt6::Test
    )

    add_test(
        NAME keyboardcontroller
        COMMAND tst_keyboardcontroller
    )
endif()

if(BUILD_TESTING)
    add_executable(tst_kwininputmethodv1backend
        tests/tst_kwininputmethodv1backend.cpp
    )

    target_link_libraries(tst_kwininputmethodv1backend
        PRIVATE
            v3keyboard-platform-kwin
            Qt6::Test
    )

    add_test(
        NAME kwininputmethodv1backend
        COMMAND tst_kwininputmethodv1backend
    )
endif()

if(BUILD_TESTING)
    add_executable(tst_kwininputmethodv1session
        tests/tst_kwininputmethodv1session.cpp
    )

    target_link_libraries(tst_kwininputmethodv1session
        PRIVATE
            v3keyboard-platform-kwin
            Qt6::Test
    )

    add_test(
        NAME kwininputmethodv1session
        COMMAND tst_kwininputmethodv1session
    )
endif()

if(BUILD_TESTING)
    add_executable(tst_kwininputmethodv1connection
        tests/tst_kwininputmethodv1connection.cpp
    )

    target_link_libraries(tst_kwininputmethodv1connection
        PRIVATE
            v3keyboard-platform-kwin
            Qt6::Test
    )

    add_test(
        NAME kwininputmethodv1connection
        COMMAND tst_kwininputmethodv1connection
    )
endif()

if(BUILD_TESTING)
    add_executable(tst_keyboarduibridge
        tests/tst_keyboarduibridge.cpp
    )

    target_link_libraries(tst_keyboarduibridge
        PRIVATE
            v3keyboard-app
            Qt6::Test
    )

    add_test(
        NAME keyboarduibridge
        COMMAND tst_keyboarduibridge
    )
endif()

if(BUILD_TESTING)
    add_executable(tst_keyboardmodel
        tests/tst_keyboardmodel.cpp
    )

    target_link_libraries(tst_keyboardmodel
        PRIVATE
            v3keyboard-core
            Qt6::Test
    )

    add_test(
        NAME keyboardmodel
        COMMAND tst_keyboardmodel
    )
endif()

if(BUILD_TESTING)
    add_executable(tst_specialkeys
        tests/tst_specialkeys.cpp
    )

    target_link_libraries(tst_specialkeys
        PRIVATE
            v3keyboard-core
            Qt6::Test
    )

    add_test(
        NAME specialkeys
        COMMAND tst_specialkeys
    )
endif()

if(BUILD_TESTING)
    add_executable(tst_keyboardlayoutmetrics
        tests/tst_keyboardlayoutmetrics.cpp
    )

    target_link_libraries(tst_keyboardlayoutmetrics
        PRIVATE
            v3keyboard-app
            Qt6::Test
    )

    add_test(
        NAME keyboardlayoutmetrics
        COMMAND tst_keyboardlayoutmetrics
    )
endif()

if(BUILD_TESTING)
    add_executable(tst_smarttyping
        tests/tst_smarttyping.cpp
    )
    target_link_libraries(tst_smarttyping PRIVATE v3keyboard-core Qt6::Test)
    add_test(NAME smarttyping COMMAND tst_smarttyping)

    add_executable(tst_clipboardemoji
        tests/tst_clipboardemoji.cpp
    )
    target_link_libraries(tst_clipboardemoji PRIVATE v3keyboard-core Qt6::Test)
    add_test(NAME clipboardemoji COMMAND tst_clipboardemoji)
endif()
