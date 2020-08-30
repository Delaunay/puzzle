# tests need source header
FIND_PACKAGE(Vulkan REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/dependencies/sdl2/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/dependencies/imgui)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/dependencies/stb)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/dependencies/spdlog/include)
INCLUDE(CTest)

# This is for QtCreator which does not add single header to the project
SET(TEST_HEADER
    forest_test.h
)

setup_target_for_coverage_gcovr_html(
    NAME coverage
    EXECUTABLE ctest -j 4)

# Add your test to CMAKE
# to run all tests run 'make test'
MACRO(TEST_MACRO NAME) # LIBRARIES
    ADD_EXECUTABLE(${NAME}_test ${NAME}_test.cpp ${TEST_HEADER})
    TARGET_LINK_LIBRARIES(${NAME}_test gtest -pthread
      editor
      glm::glm
      spdlog::spdlog
      SDL2
      Vulkan::Vulkan
      vkApplication
      nlohmann_json::nlohmann_json
    )

    # gtest need to be compiled first
    ADD_DEPENDENCIES(${NAME}_test gtest)
    ADD_DEPENDENCIES(coverage ${NAME}_test)
    ADD_TEST(
        NAME ${NAME}_test
        WORKING_DIRECTORY .
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}_test)
ENDMACRO(TEST_MACRO)

# add test here
# file_name_test.cpp ==> CBTEST_MACRO(file_name)

TEST_MACRO(puzzle)
