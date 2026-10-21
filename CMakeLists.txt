cmake_minimum_required(VERSION 3.16)
project(HoraceGoesSkiing VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Исходники в UTF-8 (русские комментарии и строки). Без этого флага Visual Studio
# на русской Windows читает их как CP1251, и русский текст в игре превращается в кракозябры.
if(MSVC)
    add_compile_options(/utf-8)
endif()

include(FetchContent)

# ---------------------------------------------------------------------------
# SFML 2.6.x (скачивается автоматически при первой конфигурации)
# Примеры на слайдах написаны под SFML 2.x — в 3.0 API другой.
# ---------------------------------------------------------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)   # статическая сборка: SFML вшивается в exe
set(SFML_BUILD_AUDIO   ON  CACHE BOOL "" FORCE)  # музыка
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.2
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(SFML)

# ---------------------------------------------------------------------------
# GoogleTest (не берите release-1.10.0 со слайда — он не дружит с новым CMake)
# ---------------------------------------------------------------------------
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # нужно для Visual Studio
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# 1) Логика игры — БЕЗ SFML. Её и тестируем.
# ---------------------------------------------------------------------------
add_library(horace_core
    src/core/Rect.cpp
    src/core/Horace.cpp
    src/core/Car.cpp
    src/core/Ambulance.cpp
    src/core/Beeper.cpp
    src/core/HighScores.cpp
    src/core/RoadLevel.cpp
    src/core/SlopeLevel.cpp
    src/core/Game.cpp
)
target_include_directories(horace_core PUBLIC src)

# ---------------------------------------------------------------------------
# 2) Сама игра: окно, ввод, отрисовка через SFML
# ---------------------------------------------------------------------------
add_executable(horace
    src/app/main.cpp
    src/app/Renderer.cpp
    src/app/SoundPlayer.cpp
)
target_link_libraries(horace PRIVATE horace_core sfml-graphics sfml-audio sfml-window sfml-system)

# Звук в SFML 2.x работает через OpenAL. На Windows рядом с horace.exe должен лежать
# openal32.dll — копируем его из SFML после каждой сборки.
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OPENAL_ARCH x64)
    else()
        set(OPENAL_ARCH x86)
    endif()
    add_custom_command(TARGET horace POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${sfml_SOURCE_DIR}/extlibs/bin/${OPENAL_ARCH}/openal32.dll"
                "$<TARGET_FILE_DIR:horace>"
        COMMENT "Copy openal32.dll next to horace.exe")
endif()

# Путь к шрифтам и картинкам — чтобы игра находила их, откуда бы её ни запустили
target_compile_definitions(horace PRIVATE
    HORACE_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
    # таблица рекордов — в папке проекта (в .gitignore: у каждого свои рекорды)
    HORACE_SCORES_FILE="${CMAKE_CURRENT_SOURCE_DIR}/highscores.txt")

# ---------------------------------------------------------------------------
# 3) Модульные тесты
# ---------------------------------------------------------------------------
enable_testing()
add_executable(horace_tests
    tests/RectTest.cpp
    tests/HoraceTest.cpp
    tests/CarTest.cpp
    tests/AmbulanceTest.cpp
    tests/BeeperTest.cpp
    tests/HighScoresTest.cpp
    tests/RoadLevelTest.cpp
    tests/SlopeLevelTest.cpp
    tests/GameTest.cpp
)
target_link_libraries(horace_tests PRIVATE horace_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(horace_tests)
