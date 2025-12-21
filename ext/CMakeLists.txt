cmake_minimum_required(VERSION 3.24) 
set(FETCHCONTENT_QUIET OFF)

include(FetchContent) 

# ---------------- GLFW ---------------- 
FetchContent_Declare( 
	glfw 
	GIT_REPOSITORY https://github.com/glfw/glfw.git 
	GIT_TAG latest 
) 

# ---------------- Vulkan Headers (Vulkan-HPP) ---------------- 
FetchContent_Declare( 
	VulkanHeaders 
	GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git 
	GIT_TAG main 
) 

# ---------------- VMA ---------------- 
FetchContent_Declare( 
	VulkanMemoryAllocator 
	GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git 
	GIT_TAG master 
) 

# ---------------- GLM ---------------- 
FetchContent_Declare( 
	glm 
	GIT_REPOSITORY https://github.com/g-truc/glm.git 
	GIT_TAG master 
) 

# ---------------- ImGui ---------------- 
FetchContent_Declare( 
	imgui 
	GIT_REPOSITORY https://github.com/ocornut/imgui.git 
	GIT_TAG master 
) 

FetchContent_MakeAvailable(glfw VulkanHeaders VulkanMemoryAllocator glm imgui) 

target_compile_definitions(glm PUBLIC
  GLM_FORCE_XYZW_ONLY
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_SILENT_WARNINGS
  GLM_ENABLE_EXPERIMENTAL
  GLM_EXT_INCLUDED
)

# Build ImGui
add_library(
	imgui_lib 
	${imgui_SOURCE_DIR}/imgui.cpp 
	${imgui_SOURCE_DIR}/imgui_draw.cpp 
	${imgui_SOURCE_DIR}/imgui_demo.cpp 
	${imgui_SOURCE_DIR}/imgui_widgets.cpp 
	${imgui_SOURCE_DIR}/imgui_tables.cpp 
	${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${VulkanHeaders_SOURCE_DIR}/include ${glfw_SOURCE_DIR}/include)
target_link_libraries(imgui_lib PUBLIC Vulkan-Headers glfw)