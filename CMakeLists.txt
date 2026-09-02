cmake_minimum_required(VERSION 3.22)

project(SlayTheSpireClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 源码包含 UTF-8 中文界面文字。
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# 优先复用仓库旁边已下载的 VS2022 x64 版 SFML 3.0.1。
set(LOCAL_SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SFML-3.0.1-vc17/SFML-3.0.1")

if(EXISTS "${LOCAL_SFML_DIR}/lib/cmake/SFML/SFMLConfig.cmake")
    set(SFML_DIR "${LOCAL_SFML_DIR}/lib/cmake/SFML")
else()
    include(FetchContent)
    FetchContent_Declare(
        sfml_binary
        URL https://github.com/SFML/SFML/releases/download/3.0.1/SFML-3.0.1-windows-vc17-64-bit.zip
        URL_HASH SHA256=2da4b95bab898686e06c6dd7f2333bcaad572cb70deb96c41d52321df90af48b
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )
    FetchContent_MakeAvailable(sfml_binary)

    set(SFML_DIR "${sfml_binary_SOURCE_DIR}/lib/cmake/SFML")
endif()
find_package(SFML 3.0.1 EXACT CONFIG REQUIRED COMPONENTS Graphics Audio)

add_library(card_system STATIC
    src/card/Card.cpp
    src/card/CardDatabase.cpp
    src/card/Deck.cpp
)

target_include_directories(card_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(combat_system STATIC
    src/combat/CombatSystem.cpp
    src/combat/Player.cpp
    src/combat/Enemy.cpp
)

target_include_directories(combat_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(combat_system PUBLIC card_system)

add_library(relic_system STATIC
    src/relic/RelicDatabase.cpp
    src/relic/RelicSystem.cpp
)

target_include_directories(relic_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(relic_system PUBLIC card_system)

add_library(event_system STATIC
    src/event/EventDatabase.cpp
    src/event/EventSystem.cpp
)

target_include_directories(event_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(map_system STATIC
    src/map/MapNode.cpp
    src/map/MapGenerator.cpp
)

target_include_directories(map_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(room_system STATIC
    src/room/RestSystem.cpp
    src/room/ShopSystem.cpp
)

target_include_directories(room_system PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(room_system PUBLIC card_system relic_system)

add_library(ui STATIC
    src/ui/CardView.cpp
    src/ui/BattleView.cpp
    src/ui/EventView.cpp
    src/ui/MainMenuView.cpp
    src/ui/RestView.cpp
    src/ui/ShopView.cpp
    src/ui/UiHelpers.cpp
)

target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ui PUBLIC
    card_system
    combat_system
    event_system
    room_system
    SFML::Graphics
    SFML::Audio
)

add_executable(SlayTheSpire
    src/main.cpp
    src/app/Game.cpp
)

target_link_libraries(SlayTheSpire PRIVATE
    combat_system
    relic_system
    room_system
    ui
    event_system
    map_system
    SFML::Graphics
    SFML::Audio
)

option(BUILD_TESTING "Build project tests" ON)
enable_testing()

if(BUILD_TESTING)
    add_executable(CardTests
        tests/card_tests.cpp
    )

    target_link_libraries(CardTests PRIVATE card_system)
    if(MSVC)
        target_compile_options(CardTests PRIVATE /utf-8)
    endif()
    add_test(NAME CardTests COMMAND CardTests)

    add_executable(CombatTests tests/combat_tests.cpp)
    target_link_libraries(CombatTests PRIVATE combat_system)
    add_test(NAME CombatTests COMMAND CombatTests)

    add_executable(RelicTests tests/relic_tests.cpp)
    target_link_libraries(RelicTests PRIVATE relic_system)
    add_test(NAME RelicTests COMMAND RelicTests)

    add_executable(RoomTests tests/room_tests.cpp)
    target_link_libraries(RoomTests PRIVATE room_system relic_system)
    add_test(NAME RoomTests COMMAND RoomTests)

    add_executable(MapTests tests/map_tests.cpp)
    target_link_libraries(MapTests PRIVATE map_system)
    add_test(NAME MapTests COMMAND MapTests)
endif()

if(WIN32)
    add_custom_command(TARGET SlayTheSpire POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:SlayTheSpire>
                $<TARGET_FILE_DIR:SlayTheSpire>
        COMMAND_EXPAND_LISTS
    )
endif()

add_custom_command(TARGET SlayTheSpire POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:SlayTheSpire>/assets"
)
