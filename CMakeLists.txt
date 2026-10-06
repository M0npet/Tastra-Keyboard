cmake_minimum_required(VERSION 3.25)

project(
    V3Keyboard
    VERSION 0.5.0
    DESCRIPTION "Privacy-first customizable virtual keyboard for Wayland"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Qml Quick Test WaylandClient)
# Qt >= 6.9 ships private modules as separate CMake packages (Arch: Qt 6.11).
# Older Qt defines the private targets together with the public module.
if(NOT TARGET Qt6::GuiPrivate)
    find_package(Qt6GuiPrivate CONFIG REQUIRED)
endif()
if(NOT TARGET Qt6::WaylandClientPrivate)
    find_package(Qt6WaylandClientPrivate CONFIG REQUIRED)
endif()
find_package(PkgConfig REQUIRED)
pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
pkg_check_modules(HUNSPELL REQUIRED IMPORTED_TARGET hunspell)
find_package(Threads REQUIRED)

set(
    INPUT_METHOD_V1_PROTOCOL
    "${WAYLAND_PROTOCOLS_DATADIR}/unstable/input-method/input-method-unstable-v1.xml"
)

add_library(v3keyboard-core STATIC
    src/core/keyboardcontroller.cpp
    src/core/keyboardmodel.cpp
    src/core/locallexicon.cpp
    src/core/capsmode.cpp
    src/core/typingengine.cpp
    src/core/clipboardhistory.cpp
    src/core/emojicatalog.cpp
)

target_include_directories(v3keyboard-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB V3KBD_DATA_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/frequency/*.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/emoji/*.tsv"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/blocklist/*.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/sounds/*.wav")
qt_add_resources(v3keyboard-core "v3keyboard-data"
    PREFIX "/v3keyboard"
    BASE data
    FILES ${V3KBD_DATA_FILES}
)

target_link_libraries(v3keyboard-core
    PUBLIC
        Qt6::Core
    PRIVATE
        PkgConfig::HUNSPELL
        Threads::Threads
)

add_library(v3keyboard-platform-kwin STATIC
    src/platform/kwin/kwininputmethodv1backend.cpp
    src/platform/kwin/kwininputmethodv1session.cpp
    src/platform/kwin/kwininputmethodv1connection.cpp
    src/platform/kwin/kwininputpanelintegration.cpp
    src/platform/kwin/kwininputpanelshellintegration.cpp
    src/platform/kwin/kwininputpanelsurface.cpp
)

# PRIVATE_CODE exists since Qt 6.8; older Qt only knows the (default) public code.
set(V3KBD_WAYLAND_CODE_VISIBILITY)
if(Qt6WaylandClient_VERSION VERSION_GREATER_EQUAL 6.8)
    set(V3KBD_WAYLAND_CODE_VISIBILITY PRIVATE_CODE)
endif()

qt_generate_wayland_protocol_client_sources(
    v3keyboard-platform-kwin
    ${V3KBD_WAYLAND_CODE_VISIBILITY}
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
    src/app/tracelog.cpp
    src/app/panelvisibility.cpp
    src/app/keyboardhider.cpp
    src/app/voicecontroller.cpp
)

target_include_directories(v3keyboard-app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(v3keyboard-app PRIVATE V3KBD_VERSION="${PROJECT_VERSION}")
target_link_libraries(v3keyboard-app
    PUBLIC
        v3keyboard-core
        Qt6::Core
        Qt6::DBus
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

    target_compile_definitions(tst_keyboarduibridge PRIVATE V3KBD_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
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
    target_compile_definitions(tst_smarttyping PRIVATE V3KBD_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    target_link_libraries(tst_smarttyping PRIVATE v3keyboard-core Qt6::Test)
    add_test(NAME smarttyping COMMAND tst_smarttyping)

    add_executable(tst_clipboardemoji
        tests/tst_clipboardemoji.cpp
    )
    target_link_libraries(tst_clipboardemoji PRIVATE v3keyboard-core Qt6::Test)
    add_test(NAME clipboardemoji COMMAND tst_clipboardemoji)
endif()

if(BUILD_TESTING)
    add_executable(tst_qmlkeyboard
        tests/tst_qmlkeyboard.cpp
    )
    target_compile_definitions(tst_qmlkeyboard PRIVATE
        V3KBD_MAIN_QML="${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Main.qml"
        V3KBD_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    qt_add_resources(tst_qmlkeyboard "tst-qmlkeyboard-icons"
        PREFIX "/v3keyboard"
        FILES
            src/ui/icons/shift.svg
            src/ui/icons/shift-lock.svg
            src/ui/icons/backspace.svg
            src/ui/icons/enter.svg
    )
    target_link_libraries(tst_qmlkeyboard PRIVATE v3keyboard-app Qt6::Quick Qt6::Qml Qt6::Test)
    add_test(NAME qmlkeyboard COMMAND tst_qmlkeyboard)
    set_tests_properties(qmlkeyboard PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

if(BUILD_TESTING)
    add_executable(tst_lexicon tests/tst_lexicon.cpp)
    target_compile_definitions(tst_lexicon PRIVATE V3KBD_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    target_link_libraries(tst_lexicon PRIVATE v3keyboard-core Qt6::Test)
    add_test(NAME lexicon COMMAND tst_lexicon)
endif()

if(BUILD_TESTING)
    add_executable(tst_voice tests/tst_voice.cpp)
    target_link_libraries(tst_voice PRIVATE v3keyboard-app Qt6::Test)
    add_test(NAME voice COMMAND tst_voice)
endif()

# Gboard "Sound on keypress": only when Qt Multimedia is installed.
find_package(Qt6 QUIET COMPONENTS Multimedia)
if(TARGET Qt6::Multimedia)
    target_link_libraries(v3keyboard-app PUBLIC Qt6::Multimedia)
    target_compile_definitions(v3keyboard-app PUBLIC V3KBD_HAVE_KEY_SOUND=1)
endif()

# Offline voice input (opt-in at install time): built only when both Qt
# Multimedia and whisper.cpp are present; otherwise the keyboard has no mic key.
option(V3KBD_VOICE "Build offline voice input if whisper.cpp and Qt Multimedia are available" ON)
set(V3KBD_HAVE_VOICE OFF)
if(V3KBD_VOICE)
    find_package(Qt6 QUIET COMPONENTS Multimedia)
    find_package(whisper CONFIG QUIET)
    if(NOT TARGET whisper)
        pkg_check_modules(WHISPER QUIET IMPORTED_TARGET whisper)
        if(TARGET PkgConfig::WHISPER)
            add_library(whisper INTERFACE IMPORTED)
            target_link_libraries(whisper INTERFACE PkgConfig::WHISPER)
        endif()
    endif()
    if(TARGET Qt6::Multimedia AND TARGET whisper)
        set(V3KBD_HAVE_VOICE ON)
        add_library(v3keyboard-voice STATIC
            src/voice/qtaudiorecorder.cpp
            src/voice/whisperrecognizer.cpp
        )
        target_include_directories(v3keyboard-voice PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
        target_link_libraries(v3keyboard-voice PUBLIC v3keyboard-app Qt6::Multimedia whisper)
        target_link_libraries(v3-keyboard PRIVATE v3keyboard-voice)
        target_compile_definitions(v3-keyboard PRIVATE V3KBD_HAVE_VOICE=1)
        if(BUILD_TESTING)
            add_executable(tst_voiceadapters tests/tst_voiceadapters.cpp)
            target_link_libraries(tst_voiceadapters PRIVATE v3keyboard-voice Qt6::Test)
            add_test(NAME voiceadapters COMMAND tst_voiceadapters)
        endif()
    endif()
endif()
message(STATUS "V3 Keyboard offline voice input: ${V3KBD_HAVE_VOICE}")

if(BUILD_TESTING)
    add_executable(tst_typingstress tests/tst_typingstress.cpp)
    target_compile_definitions(tst_typingstress PRIVATE V3KBD_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    target_link_libraries(tst_typingstress PRIVATE v3keyboard-core Qt6::Test)
    add_test(NAME typingstress COMMAND tst_typingstress)
endif()

option(V3KBD_BENCHMARKS "Build developer benchmarks" OFF)
if(V3KBD_BENCHMARKS)
    add_executable(bench_keystroke tools/bench_keystroke.cpp)
    target_link_libraries(bench_keystroke PRIVATE v3keyboard-app)
endif()

if(BUILD_TESTING)
    add_executable(tst_panelvisibility tests/tst_panelvisibility.cpp)
    target_link_libraries(tst_panelvisibility PRIVATE v3keyboard-app Qt6::Test)
    add_test(NAME panelvisibility COMMAND tst_panelvisibility)
endif()

if(BUILD_TESTING)
    add_executable(tst_capsmode tests/tst_capsmode.cpp)
    target_link_libraries(tst_capsmode PRIVATE v3keyboard-core Qt6::Test)
    add_test(NAME capsmode COMMAND tst_capsmode)
endif()
