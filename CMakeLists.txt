cmake_minimum_required(VERSION 3.24) 
project(App) 
set(CMAKE_CXX_STANDARD 23) 
add_subdirectory(ext) 

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h) 

add_executable(App ${APP_SOURCES}) 

target_include_directories(App PUBLIC src) 

# Link all deps 
target_link_libraries(App glfw VulkanMemoryAllocator glm imgui_lib ) 

# Enable Vulkan include path (Vulkan-Headers) 
target_include_directories(App PUBLIC ${VulkanHeaders_SOURCE_DIR}/include ) 

# Windows Vulkan SDK auto-detection 
find_package(Vulkan REQUIRED) 

target_compile_definitions(App PRIVATE VK_NO_PROTOTYPES) 
target_link_libraries(App Vulkan::Vulkan)