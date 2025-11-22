# vulkan is and low level graphics API.
# we will simplify its usage creating an INTERFACE library
# this means we are only going to provide include directories and link libraries as
# INTERFACE as well and not compiling anything.
find_package(Vulkan REQUIRED)
add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE Vulkan::Vulkan)

# glslangValidator is a tool for compiling GLSL shaders to SPIR-V.
# this tool is usually included in the Vulkan SDK. 
# On Windows case its on LunarG SDK.
# On Linux you might need to install it separately.
# On Ubuntu you can install it via:
# sudo apt-get install glslang-tools
# On Arch Linux:
# sudo pacman -S glslang
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# fastgltf is a glTF 2.0 library for C++. It loads models quickly and efficiently.
# it already has a CMakeLists.txt file so we can just add it as a subdirectory.
add_subdirectory(fastgltf EXCLUDE_FROM_ALL)

# fmt is a modern formatting library for C++.
# it also has a CMakeLists.txt file so we can add it as a subdirectory.
add_subdirectory(fmt EXCLUDE_FROM_ALL)

# SDL2 is a cross-platform development library for handling graphics, input, and audio.
# we add it as a subdirectory as well, and we also link libraries so we load everything
# includes and links from the 2 libraries provided by SDL2(SDL2 and SDL2main).
add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_library(sdl2 INTERFACE)
target_link_libraries(sdl2 INTERFACE SDL2::SDL2 SDL2::SDL2main)

# GLM library provides math functions for graphics programming
# by default GLM is header only so we create an INTERFACE library
# this means we are only going to provide include directories as 
# INTERFACE as well and not compiling anything.
# expose glma folder as a include directory to be used by anyone that includes glm
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# Vulkan Memory Allocator (VMA) provides a simple and efficient way
# to manage vulkan memory allocations. This is also a header only library
# so we are going to need another interface library for it.
# expose vma folder as a include directory to be used by anyone that includes vma
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma)

# stb_image is a header only library for loading images
# we create an interface library for it as well
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE stb_image)

# SPIRV-Reflect is a library for parsing SPIR-V shader bytecode.
# it is a STATIC library containing only one file and should expose its include directory
# any library using it should also link against Vulkan
add_library(spirv_reflect STATIC SPIRV-Reflect/spirv_reflect.c)
target_include_directories(spirv_reflect PUBLIC SPIRV-Reflect)
target_link_libraries(spirv_reflect PUBLIC vulkan)

# VkBootstrap is a library that simplifies Vulkan initialization.
# it is a STATIC library containing two files and should expose its include directory
# any library using it should also link against Vulkan and dl on UNIX systems
add_library(vkbootstrap STATIC)
target_sources(vkbootstrap PRIVATE vkbootstrap/VkBootstrap.h vkbootstrap/VkBootstrap.cpp)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
set_property(TARGET vkbootstrap PROPERTY CXX_STANDARD 20)

# ImGui is a popular immediate mode GUI library.
# it is a STATIC library containing multiple files and should expose its include directory
# any library using it should also link against Vulkan and SDL2
add_library(imgui STATIC)
target_include_directories(imgui PUBLIC imgui)
target_sources(imgui PRIVATE 
  imgui/imgui.h
  imgui/imgui.cpp

  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_tables.cpp

  imgui/imgui_impl_vulkan.cpp
  imgui/imgui_impl_sdl2.cpp
)
target_link_libraries(imgui PUBLIC vulkan sdl2)

#add_library(sdl2 INTERFACE)
#target_include_directories(sdl2 INTERFACE $ENV{VULKAN_SDK}/Include/SDL2)
#target_link_directories(sdl2 INTERFACE $ENV{VULKAN_SDK}/Lib)
#target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)
