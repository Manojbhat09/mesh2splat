cmake_minimum_required(VERSION 3.10)
project(Mesh2Splat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRD_PARTY_DIR         ${CMAKE_SOURCE_DIR}/thirdParty)
set(GLFW_DIR                ${THIRD_PARTY_DIR}/glfw)
set(GLEW_DIR                ${THIRD_PARTY_DIR}/glew)
set(GLM_DIR                 ${THIRD_PARTY_DIR}/glm)
set(IMGUI_DIR               ${THIRD_PARTY_DIR}/imgui)
set(IGFD_DIR                ${THIRD_PARTY_DIR}/ImGuiFileDialog)
set(IMGUIZMO_DIR            ${THIRD_PARTY_DIR}/imguizmo)
set(XATLAS_DIR              ${THIRD_PARTY_DIR}/xatlas)

include_directories(${GLFW_DIR}/include)
include_directories(${GLEW_DIR}/include)
include_directories(${GLM_DIR})
include_directories(${IMGUI_DIR})
include_directories(${IGFD_DIR})
include_directories(${IMGUIZMO_DIR})
include_directories(${XATLAS_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${THIRD_PARTY_DIR})
include_directories(/home/jules/.pyenv/versions/3.12.11/include/python3.12)
include_directories(/home/jules/.pyenv/versions/3.12.11/lib/python3.12/site-packages/pybind11/include)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python_INCLUDE_DIRS})

set(CORE_SOURCES
    src/pybind11_bindings.cpp
    src/HeadlessConverter.cpp
    src/utils/SceneManager.cpp
    src/utils/glUtils.cpp
    src/utils/ShaderRegistry.cpp
    src/utils/normalizedUvUnwrapping.cpp
    src/parsers/parsers.cpp
    src/renderer/renderPasses/ConversionPass.cpp
    src/renderer/renderPasses/DepthPrepass.cpp
    src/renderer/renderPasses/GaussianRelightingPass.cpp
    src/renderer/renderPasses/GaussianShadowPass.cpp
    src/renderer/renderPasses/GaussianSplattingPass.cpp
    src/renderer/renderPasses/GaussiansPrepass.cpp
    src/renderer/renderPasses/RadixSortPass.cpp
    src/glewGlfwHandlers/glewGlfwHandler.cpp
    src/imGuiUi/ImGuiUI.cpp
    src/implementationFiles/stb_image_impl.cpp
    src/implementationFiles/stb_image_resize_impl.cpp
    src/implementationFiles/stb_image_write_impl.cpp
    src/implementationFiles/tiny_gltf_impl.cpp
)

add_library(mesh2splat_core SHARED ${CORE_SOURCES})
target_link_libraries(mesh2splat_core PRIVATE ${Python_LIBRARIES})
set_target_properties(mesh2splat_core PROPERTIES PREFIX "")
set_target_properties(mesh2splat_core PROPERTIES SUFFIX ".so")


target_compile_definitions(mesh2splat_core PRIVATE GLEW_STATIC)

if (WIN32)
    target_link_libraries(mesh2splat_core PRIVATE
        ${GLFW_DIR}/lib-vc2022/glfw3.lib
        ${GLEW_DIR}/lib/Release/x64/glew32s.lib
        opengl32
        ${Python_LIBRARIES})
else()
    target_link_libraries(mesh2splat_core PRIVATE glfw glew GL ${Python_LIBRARIES})
endif()
