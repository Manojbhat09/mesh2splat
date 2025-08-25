#include "HeadlessConverter.hpp"
#include "utils/params.hpp"
#include "utils/SceneManager.hpp"
#include "renderer/renderPasses/ConversionPass.hpp"
#include "glewGlfwHandlers/glewGlfwHandler.hpp"
#include "utils/glUtils.hpp"
#include "RenderContext.hpp"
#include <iostream>

HeadlessConverter::HeadlessConverter() : window(nullptr) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(640, 480, "Headless", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return;
    }
}

HeadlessConverter::~HeadlessConverter() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

bool HeadlessConverter::convert(const std::string& glb_path, const std::string& output_ply_path, float sampling_density, unsigned int ply_format) {
    if (!window) {
        return false;
    }

    RenderContext renderContext;

    glUtils::initializeShaderLocations();
    glUtils::initializeShaderFileMonitoring(renderContext.shaderRegistry);
    renderContext.shaderRegistry.reloadModifiedShaders(true);

    // Estimate resolution based on sampling density. This is a heuristic.
    // A density of 1.0 will correspond to a resolution of 1024.
    renderContext.resolutionTarget = static_cast<int>(1024 * sampling_density);
    renderContext.gaussianStd = 0.5f;

    glUtils::setupAtomicCounter(renderContext.atomicCounterBufferConversionPass);

    SceneManager sceneManager(renderContext);

    size_t lastSlashPos = glb_path.find_last_of("/\\");
    std::string parentFolder = (lastSlashPos == std::string::npos) ? "" : glb_path.substr(0, lastSlashPos + 1);

    if (!sceneManager.loadModel(glb_path, parentFolder)) {
        std::cerr << "Failed to load model: " << glb_path << std::endl;
        return false;
    }

    ConversionPass conversionPass;
    conversionPass.execute(renderContext);

    sceneManager.exportPly(output_ply_path, ply_format);

    return true;
}
