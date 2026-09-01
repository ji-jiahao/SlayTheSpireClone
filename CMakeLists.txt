cmake_minimum_required(VERSION 3.22)

project(SlayTheSpireClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Download the official VS2022 x64 package; no personal install path is needed.
include(FetchContent)
FetchContent_Declare(
    sfml_binary
    URL https://github.com/SFML/SFML/releases/download/3.0.1/SFML-3.0.1-windows-vc17-64-bit.zip
    URL_HASH SHA256=2da4b95bab898686e06c6dd7f2333bcaad572cb70deb96c41d52321df90af48b
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(sfml_binary)

set(SFML_DIR "${sfml_binary_SOURCE_DIR}/lib/cmake/SFML")
find_package(SFML 3.0.1 EXACT CONFIG REQUIRED COMPONENTS Graphics)

add_library(card_system STATIC
    src/card/Card.cpp
    src/card/CardDatabase.cpp
    src/card/Deck.cpp
)

target_include_directories(card_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(SlayTheSpire
    src/main.cpp
)

target_link_libraries(SlayTheSpire PRIVATE card_system SFML::Graphics)

option(BUILD_TESTING "Build the card module tests" ON)
enable_testing()

if(BUILD_TESTING)
    add_executable(CardTests
        tests/card_tests.cpp
    )

    target_link_libraries(CardTests PRIVATE card_system)
    add_test(NAME CardTests COMMAND CardTests)
endif()

# Copy exactly the runtime DLLs required by the executable.
if(WIN32)
    add_custom_command(TARGET SlayTheSpire POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:SlayTheSpire>
                $<TARGET_FILE_DIR:SlayTheSpire>
        COMMAND_EXPAND_LISTS
    )
endif()
