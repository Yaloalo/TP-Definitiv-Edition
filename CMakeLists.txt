cmake_minimum_required(VERSION 3.26)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED ENV{CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type")
endif ()

project(tp_remaster VERSION 0.10.0 LANGUAGES CXX)

# Must match the Dusklight release the mod runs on: game structs and symbols are version-specific.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "v2.0.3")
endif ()
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

configure_file(mod.json.in "${CMAKE_CURRENT_BINARY_DIR}/mod.json" @ONLY NEWLINE_STYLE LF)

file(GLOB ENEMY_PROFILE_SOURCES CONFIGURE_DEPENDS src/combat/enemy_slow_motion/*.cpp)

add_mod(tp_remaster
        FEATURES game fmt
        SOURCES
            src/mod.cpp
            src/core/tuning.cpp
            src/core/difficulty.cpp
            src/core/player.cpp
            src/core/hooks.cpp
            src/progression/rewards.cpp
            src/progression/growth.cpp
            src/progression/pause.cpp
            src/progression/spring.cpp
            src/progression/collections.cpp
            src/healing/flask.cpp
            src/healing/rest_stones.cpp
            src/healing/wolf.cpp
            src/healing/snowpeak_hearts.cpp
            src/resources/ammo.cpp
            src/economy/merchants.cpp
            src/economy/currency.cpp
            src/economy/rare.cpp
            src/economy/conversations.cpp
            src/combat/damage.cpp
            src/combat/stamina.cpp
            src/combat/dodge.cpp
            src/combat/manual_shield.cpp
            src/combat/enemy_hard_mode.cpp
            src/combat/boss_hard_mode.cpp
            src/combat/enemy_slow_motion.cpp
            ${ENEMY_PROFILE_SOURCES}
            src/hud/stamina_bar.cpp
            src/hud/idle_fade.cpp
            src/hud/damage_feedback.cpp
            src/movement/sprint.cpp
            src/movement/traversal.cpp
            src/movement/quality.cpp
            src/movement/responsiveness.cpp
            src/movement/crawl.cpp
            src/movement/horse.cpp
            src/pacing/quality.cpp
            src/pacing/tears.cpp
            src/input/mouse_controls.cpp
            src/presentation/texture_packs.cpp
            src/presentation/controller_ui.cpp
            src/presentation/compass.cpp
            src/presentation/compass_module.cpp
            src/items/unrestricted.cpp
        MOD_JSON "${CMAKE_CURRENT_BINARY_DIR}/mod.json"
        RES_DIR res
)
target_include_directories(tp_remaster PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

include(CTest)
if (BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING AND NOT MSVC)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_executable(enemy_timing_test tests/enemy_slow_motion_timing.cpp)
    add_test(NAME enemy_timing COMMAND enemy_timing_test)
    add_test(NAME enemy_death_recovery
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/enemy_hard_mode_death_test.py")
    add_test(NAME manual_shield
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/manual_shield_test.py")
    add_test(NAME launch_folder
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/prepare_mods_test.py")
    add_executable(progression_test tests/progression_model.cpp)
    target_include_directories(progression_test PRIVATE src)
    target_compile_options(progression_test PRIVATE -UNDEBUG)
    add_test(NAME progression_save_model COMMAND progression_test)
    add_test(NAME flask_callbacks
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/flask_callbacks_test.py")
    add_test(NAME currency_and_pacing
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/currency_pacing_test.py")
    add_executable(resources_models_test tests/resources_models.cpp)
    target_include_directories(resources_models_test PRIVATE src)
    target_compile_options(resources_models_test PRIVATE -UNDEBUG)
    add_test(NAME resources_and_relic_models COMMAND resources_models_test)
    add_test(NAME wolf_resources_and_rewards
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/new_gameplay_callbacks_test.py")
    add_test(NAME rest_stones_and_emergency_recovery
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/rest_stones_test.py")
    add_executable(movement_hud_models_test tests/movement_hud_models.cpp)
    target_include_directories(movement_hud_models_test PRIVATE src)
    target_compile_options(movement_hud_models_test PRIVATE -UNDEBUG)
    add_test(NAME movement_hud_models COMMAND movement_hud_models_test)
    add_test(NAME movement_hud_callbacks
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/movement_hud_callbacks_test.py")
    add_test(NAME epona_handling
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/horse_callbacks_test.py")
    add_test(NAME story_progression
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/story_progression_test.py")
    add_test(NAME qa_tooling
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/qa_tools_test.py")
    add_test(NAME shared_growth
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/growth_callbacks_test.py")
    add_test(NAME pause_progression
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/pause_progression_test.py")
    add_test(NAME spring_redistribution
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/spring_callbacks_test.py")
    add_test(NAME rare_economy
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/rare_economy_test.py")
    add_test(NAME collection_rewards
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/collection_callbacks_test.py")
    add_test(NAME npc_conversations
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/conversation_callbacks_test.py")
    # Keep assertions enabled in RelWithDebInfo/Release test binaries.
    target_compile_options(enemy_timing_test PRIVATE -UNDEBUG)

    # Always run, even when no compilation was necessary. Depend on packaging
    # and every compiled fixture so ctest cannot see an old package/test binary.
    set(DUSKLIGHT_BINARY "$ENV{DUSKLIGHT_BINARY}" CACHE FILEPATH "Extracted pinned Dusklight release executable for QA")
    option(TP_QA_REQUIRE_HOST "Fail QA if exact-host ABI validation is unavailable" OFF)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_qa_arguments --build "${CMAKE_CURRENT_BINARY_DIR}" --mod "$<TARGET_FILE:tp_remaster>"
            --strip "${CMAKE_STRIP}" --config "$<CONFIG>")
        if (DUSKLIGHT_BINARY)
            list(APPEND _qa_arguments --host "${DUSKLIGHT_BINARY}")
        endif ()
        if (TP_QA_REQUIRE_HOST)
            list(APPEND _qa_arguments --require-host)
        endif ()
        add_custom_target(qa ALL
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa.py" ${_qa_arguments}
            DEPENDS tp_remaster_package enemy_timing_test progression_test resources_models_test movement_hud_models_test
            COMMENT "Running Definitive Edition automated QA gate"
            USES_TERMINAL VERBATIM)
    else ()
        add_custom_target(qa ALL
            COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_CURRENT_BINARY_DIR}" --output-on-failure --timeout 120 -C "$<CONFIG>"
            DEPENDS tp_remaster_package enemy_timing_test progression_test resources_models_test movement_hud_models_test
            COMMENT "Running regression fixtures (host ABI audit currently supports Linux ELF only)"
            USES_TERMINAL VERBATIM)
    endif ()
endif ()
