# Add source to this project's executable.

set(GAME_SOURCES 
  initial.cpp
  vulkanrender.cpp
  imguiloader.cpp
  sdlwindow.cpp
)

set(GAME_PRECOMPILED_HEADERS
  <optional>
  <vector>
  <memory>
  <string>
  <vector>
  <unordered_map>
  <glm/mat4x4.hpp>
  <glm/vec4.hpp>
  <vulkan/vulkan.h>
)

set(GAME_LIBRARIES
  vulkan
  sdl2
  fastgltf
  fmt
  glm
  vma
  stb_image
  vkbootstrap
  spirv_reflect
  imgui
)

set (GAME_DEFINES
  GLM_FORCE_DEPTH_ZERO_TO_ONE
)

add_executable(game)
target_sources(game PRIVATE ${GAME_SOURCES})
set_property(TARGET game PROPERTY CXX_STANDARD 20)
target_compile_definitions(game PUBLIC ${GAME_DEFINES})
target_include_directories(game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(game PUBLIC ${GAME_LIBRARIES})
target_precompile_headers(game PUBLIC ${GAME_PRECOMPILED_HEADERS})
add_dependencies(game Shaders)

if(WIN32)
  set_target_properties(game PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")

  add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:game> $<TARGET_FILE_DIR:game>
    COMMAND_EXPAND_LISTS
  )
endif()
