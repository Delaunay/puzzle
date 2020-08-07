# better way ?
INCLUDE_DIRECTORIES(.)

# Source files
# ====================================
#           (Glob is not recommended)
#   file(GLOB APL_SRC *.cc)
#   file(GLOB_RECURSE APL_SRC *.cc)

FIND_PACKAGE(Vulkan REQUIRED)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/dependencies/sdl2/include)

IF (${Vulkan_FOUND})
  MESSAGE(STATUS "Found Vulkan")
ELSE ()
  MESSAGE(STATUS "Vulkan not found")
ENDIF()

#  main executable
# ==========================

ADD_SUBDIRECTORY(application)

ADD_EXECUTABLE(main main.cpp)
TARGET_LINK_LIBRARIES(main spdlog::spdlog SDL2 Vulkan::Vulkan vkApplication)


# Find Shaders and compile them
FILE(GLOB_RECURSE SHADERS_SRC *.frag *.vert)
SET(SHADER_COMPILER /opt/vulkan/1.2.148.0/x86_64/bin/glslc)

SET(SHADER_OBJ "")

FUNCTION(compile_shader SHADER)
  get_filename_component(SHADER_FILE ${SHADER} NAME)
  FILE(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")
  SET(SHADER_OUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_FILE}.spv")

  ADD_CUSTOM_COMMAND(
    TARGET main
    PRE_BUILD
    COMMAND ${SHADER_COMPILER} ${SHADER} -g -o ${SHADER_OUT}
    DEPENDS ${SHADER}
    COMMENT "Building ${SHADER}.spv"
  )
  LIST(APPEND SHADER_OBJ ${SHADER_OUT})
ENDFUNCTION()


ADD_CUSTOM_TARGET(shaders ALL SOURCES ${SHADERS_SRC})
FOREACH(SHADER ${SHADERS_SRC})
  compile_shader(${SHADER})
  MESSAGE(STATUS "Generating build commands for ${SHADER}.spv")
ENDFOREACH()



