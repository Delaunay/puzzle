# better way ?
INCLUDE_DIRECTORIES(.)

# Source files
# ====================================
#           (Glob is not recommended)
#   file(GLOB APL_SRC *.cc)
#   file(GLOB_RECURSE APL_SRC *.cc)

FIND_PACKAGE(Vulkan REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/dependencies/sdl2/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/dependencies/imgui)


IF (${Vulkan_FOUND})
  MESSAGE(STATUS "Found Vulkan")
ELSE ()
  MESSAGE(STATUS "Vulkan not found")
ENDIF()

#  main executable
# ==========================

ADD_SUBDIRECTORY(application)
# ADD_SUBDIRECTORY(node-editor)

ADD_EXECUTABLE(main main.cpp)
TARGET_LINK_LIBRARIES(main glm::glm spdlog::spdlog SDL2 Vulkan::Vulkan vkApplication)


# Find Shaders and compile them
# move it out into a cmake module
# ===============================
FILE(GLOB_RECURSE SHADERS_SRC *.frag *.vert)
SET(SHADER_COMPILER /opt/vulkan/1.2.148.0/x86_64/bin/glslc)

FUNCTION(add_shader SHADER)
  # Get filename only
  get_filename_component(SHADER_FILE_NAME ${SHADER} NAME)

  # Get relative path to shader
  FILE(RELATIVE_PATH SHADER_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${SHADER})

  # Get relative directories
  get_filename_component(SHADER_PATH ${SHADER_FILE_PATH} DIRECTORY)

  # make the shader folder structure mirror the build folder
  # so the path is consistent between source and build
  # later when we will zip all shaders together for fast loading
  FILE(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_PATH}")
  SET(SHADER_OUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_PATH}/${SHADER_FILE_NAME}.spv")

  ADD_CUSTOM_TARGET(
    shader_${SHADER_FILE_NAME}
    COMMAND ${SHADER_COMPILER} ${SHADER} -g -o ${SHADER_OUT}
    DEPENDS ${SHADER}
    BYPRODUCTS ${SHADER_OUT}
    COMMENT "Building ${SHADER}.spv"
  )
  ADD_DEPENDENCIES(main shader_${SHADER_FILE_NAME})
ENDFUNCTION()


ADD_CUSTOM_TARGET(shaders ALL SOURCES ${SHADERS_SRC})
FOREACH(SHADER ${SHADERS_SRC})
  add_shader(${SHADER})
ENDFOREACH()



