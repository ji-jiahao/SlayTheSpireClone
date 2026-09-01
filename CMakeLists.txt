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
find_package(SFML 3.0.1 EXACT CONFIG REQUIRED COMPONENTS Graphics)

add_library(card_system STATIC
    src/card/Card.cpp
    src/card/CardDatabase.cpp
    src/card/Deck.cpp
)

target_include_directories(card_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(ui STATIC
    src/ui/CardView.cpp
)

target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ui PUBLIC card_system SFML::Graphics)

add_executable(SlayTheSpire
    src/main.cpp
)

target_link_libraries(SlayTheSpire PRIVATE card_system ui SFML::Graphics)

option(BUILD_TESTING "Build the card module tests" ON)
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
