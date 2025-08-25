#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/functional.h>
#include <cstdlib>
#include <ctime>

#include "parsers/parsers.hpp"
#include "utils/utils_simplified.hpp"
#include "../thirdParty/tiny_gltf.h"

namespace py = pybind11;

// Forward declarations
class Mesh2SplatConverter {
private:
    // Internal state for conversion
    std::vector<utils::GaussianDataSSBO> gaussians_;
    float scale_multiplier_ = 1.0f;
    
public:
    Mesh2SplatConverter() {
        // Seed random number generator for gaussian positioning
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }
    
    // Convert GLB mesh to Gaussian splats
    bool convert_glb_to_gaussians(const std::string& glb_path, float sampling_density = 1.0f) {
        try {
            // Load GLB file using tiny_gltf
            tinygltf::Model model;
            tinygltf::TinyGLTF loader;
            std::string err;
            std::string warn;
            
            bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, glb_path);
            if (!ret) {
                py::print("Failed to load GLB file: ", err);
                return false;
            }
            
            if (!warn.empty()) {
                py::print("GLB loading warnings: ", warn);
            }
            
            // Clear existing gaussians
            gaussians_.clear();
            
            // Process each mesh in the GLB file
            for (const auto& mesh : model.meshes) {
                for (const auto& primitive : mesh.primitives) {
                    // Get vertex positions
                    const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
                    const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
                    const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
                    const float* positions = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);
                    
                    // Get vertex normals (if available)
                    std::vector<glm::vec3> normals;
                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                        const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                        const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
                        const tinygltf::Buffer& normBuffer = model.buffers[normBufferView.buffer];
                        const float* normalData = reinterpret_cast<const float*>(&normBuffer.data[normBufferView.byteOffset + normAccessor.byteOffset]);
                        
                        for (size_t i = 0; i < normAccessor.count; ++i) {
                            normals.push_back(glm::vec3(normalData[i * 3], normalData[i * 3 + 1], normalData[i * 3 + 2]));
                        }
                    }
                    
                    // Get texture coordinates (if available)
                    std::vector<glm::vec2> texCoords;
                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor& texAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                        const tinygltf::BufferView& texBufferView = model.bufferViews[texAccessor.bufferView];
                        const tinygltf::Buffer& texBuffer = model.buffers[texBufferView.buffer];
                        const float* texData = reinterpret_cast<const float*>(&texBuffer.data[texBufferView.byteOffset + texAccessor.byteOffset]);
                        
                        for (size_t i = 0; i < texAccessor.count; ++i) {
                            texCoords.push_back(glm::vec2(texData[i * 2], texData[i * 2 + 1]));
                        }
                    }
                    
                    // Get indices for triangulation
                    std::vector<uint32_t> indices;
                    if (primitive.indices >= 0) {
                        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                        const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                        const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
                        
                        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                            const uint16_t* indexData = reinterpret_cast<const uint16_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
                            for (size_t i = 0; i < indexAccessor.count; ++i) {
                                indices.push_back(indexData[i]);
                            }
                        } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                            const uint32_t* indexData = reinterpret_cast<const uint32_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
                            for (size_t i = 0; i < indexAccessor.count; ++i) {
                                indices.push_back(indexData[i]);
                            }
                        }
                    }
                    
                    // Calculate number of gaussians based on sampling density
                    size_t numVertices = posAccessor.count;
                    size_t numGaussians = static_cast<size_t>(numVertices * sampling_density);
                    
                    // Generate gaussians from mesh vertices
                    for (size_t i = 0; i < numGaussians; ++i) {
                        utils::GaussianDataSSBO gaussian;
                        
                        // Sample vertex position (with some randomization for density)
                        size_t vertexIndex = i % numVertices;
                        if (sampling_density > 1.0f && i >= numVertices) {
                            // Add some randomization for higher density
                            float randX = static_cast<float>(rand()) / RAND_MAX * 0.01f;
                            float randY = static_cast<float>(rand()) / RAND_MAX * 0.01f;
                            float randZ = static_cast<float>(rand()) / RAND_MAX * 0.01f;
                            
                            gaussian.position = glm::vec4(
                                positions[vertexIndex * 3] + randX,
                                positions[vertexIndex * 3 + 1] + randY,
                                positions[vertexIndex * 3 + 2] + randZ,
                                1.0f
                            );
                        } else {
                            gaussian.position = glm::vec4(
                                positions[vertexIndex * 3],
                                positions[vertexIndex * 3 + 1],
                                positions[vertexIndex * 3 + 2],
                                1.0f
                            );
                        }
                        
                        // Set normal (use vertex normal or calculate from nearby vertices)
                        if (vertexIndex < normals.size()) {
                            gaussian.normal = glm::vec4(normals[vertexIndex], 0.0f);
                        } else {
                            // Default normal pointing up
                            gaussian.normal = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                        }
                        
                        // Extract material and color information
                        glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                        glm::vec4 pbr = glm::vec4(0.0f, 0.5f, 0.0f, 0.0f);
                        
                        // Try to get material information
                        if (primitive.material >= 0 && primitive.material < model.materials.size()) {
                            const auto& material = model.materials[primitive.material];
                            
                            // Get base color factor
                            if (material.values.find("baseColorFactor") != material.values.end()) {
                                const auto& baseColor = material.values.at("baseColorFactor");
                                if (baseColor.number_array.size() >= 3) {
                                    color.r = static_cast<float>(baseColor.number_array[0]);
                                    color.g = static_cast<float>(baseColor.number_array[1]);
                                    color.b = static_cast<float>(baseColor.number_array[2]);
                                    color.a = baseColor.number_array.size() > 3 ? static_cast<float>(baseColor.number_array[3]) : 1.0f;
                                }
                            }
                            
                            // Get metallic and roughness factors
                            if (material.values.find("metallicFactor") != material.values.end()) {
                                pbr.x = static_cast<float>(material.values.at("metallicFactor").Factor());
                            }
                            if (material.values.find("roughnessFactor") != material.values.end()) {
                                pbr.y = static_cast<float>(material.values.at("roughnessFactor").Factor());
                            }
                            
                            // Try to get vertex colors if available
                            if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
                                const tinygltf::Accessor& colorAccessor = model.accessors[primitive.attributes.at("COLOR_0")];
                                const tinygltf::BufferView& colorBufferView = model.bufferViews[colorAccessor.bufferView];
                                const tinygltf::Buffer& colorBuffer = model.buffers[colorBufferView.buffer];
                                const float* colorData = reinterpret_cast<const float*>(&colorBuffer.data[colorBufferView.byteOffset + colorAccessor.byteOffset]);
                                
                                if (vertexIndex < colorAccessor.count) {
                                    if (colorAccessor.type == TINYGLTF_TYPE_VEC3) {
                                        color.r = colorData[vertexIndex * 3];
                                        color.g = colorData[vertexIndex * 3 + 1];
                                        color.b = colorData[vertexIndex * 3 + 2];
                                    } else if (colorAccessor.type == TINYGLTF_TYPE_VEC4) {
                                        color.r = colorData[vertexIndex * 4];
                                        color.g = colorData[vertexIndex * 3 + 1];
                                        color.b = colorData[vertexIndex * 4 + 2];
                                        color.a = colorData[vertexIndex * 4 + 3];
                                    }
                                }
                            }
                        }
                        
                        gaussian.color = color;
                        gaussian.pbr = pbr;
                        
                        // Calculate proper scale based on mesh density and local geometry
                        float localScale = 0.0f;
                        
                        // Use triangle area or vertex density for scale
                        if (vertexIndex < normals.size()) {
                            // Scale based on normal magnitude and sampling density
                            float normalMag = glm::length(glm::vec3(normals[vertexIndex]));
                            localScale = normalMag * 0.1f; // Base scale from normal
                        }
                        
                        // Apply sampling density scaling
                        if (sampling_density > 1.0f) {
                            localScale *= (1.0f / sampling_density);
                        }
                        
                        // Ensure minimum scale for visibility
                        localScale = std::max(localScale, 0.05f);
                        
                        // Apply user scale multiplier
                        localScale *= scale_multiplier_;
                        
                        // Set anisotropic scale (different in normal direction)
                        glm::vec3 normalVec = glm::vec3(gaussian.normal);
                        glm::vec3 tangent1 = glm::normalize(glm::cross(normalVec, glm::vec3(1.0f, 0.0f, 0.0f)));
                        if (glm::length(tangent1) < 0.1f) {
                            tangent1 = glm::normalize(glm::cross(normalVec, glm::vec3(0.0f, 1.0f, 0.0f)));
                        }
                        glm::vec3 tangent2 = glm::normalize(glm::cross(normalVec, tangent1));
                        
                        // Scale in tangent directions (surface-aligned)
                        float tangentScale = localScale * 0.8f;
                        float normalScale = localScale * 1.2f; // Slightly larger in normal direction
                        
                        gaussian.scale = glm::vec4(tangentScale, tangentScale, normalScale, 1.0f);
                        
                        // Set rotation to align with surface normal
                        // Convert normal to quaternion rotation
                        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                        glm::vec3 normal = glm::normalize(glm::vec3(gaussian.normal));
                        
                        if (glm::abs(glm::dot(normal, up)) < 0.99f) {
                            glm::vec3 axis = glm::normalize(glm::cross(up, normal));
                            float angle = glm::acos(glm::dot(up, normal));
                            gaussian.rotation = glm::vec4(
                                glm::cos(angle * 0.5f),
                                axis.x * glm::sin(angle * 0.5f),
                                axis.y * glm::sin(angle * 0.5f),
                                axis.z * glm::sin(angle * 0.5f)
                            );
                        } else {
                            // Normal is close to up vector, use identity
                            gaussian.rotation = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                        }
                        
                        gaussians_.push_back(gaussian);
                    }
                }
            }
            
            py::print("Successfully converted GLB to ", gaussians_.size(), " gaussians");
            
            // Debug: Show scale and rotation info
            if (!gaussians_.empty()) {
                const auto& firstGaussian = gaussians_[0];
                py::print("First gaussian scale: ", firstGaussian.scale.x, ", ", firstGaussian.scale.y, ", ", firstGaussian.scale.z);
                py::print("First gaussian rotation: ", firstGaussian.rotation.w, ", ", firstGaussian.rotation.x, ", ", firstGaussian.rotation.y, ", ", firstGaussian.rotation.z);
                py::print("First gaussian normal: ", firstGaussian.normal.x, ", ", firstGaussian.normal.y, ", ", firstGaussian.normal.z);
            }
            
            // Debug: Show material information
            if (!model.materials.empty()) {
                py::print("Found ", model.materials.size(), " materials in GLB file");
                for (size_t i = 0; i < model.materials.size(); ++i) {
                    const auto& material = model.materials[i];
                    py::print("  Material ", i, ": ", material.name);
                    
                    if (material.values.find("baseColorFactor") != material.values.end()) {
                        const auto& baseColor = material.values.at("baseColorFactor");
                        if (baseColor.number_array.size() >= 3) {
                            py::print("    Base color: ", baseColor.number_array[0], ", ", baseColor.number_array[1], ", ", baseColor.number_array[2]);
                        }
                    }
                }
            }
            
            return true;
            
        } catch (const std::exception& e) {
            py::print("Error converting GLB to gaussians: ", e.what());
            return false;
        }
    }
    
    // Save gaussians to PLY file
    bool save_to_ply(const std::string& output_path, unsigned int format = 0) {
        try {
            if (gaussians_.empty()) {
                py::print("No gaussians to save. Convert a mesh first.");
                return false;
            }
            
            parsers::savePlyVector(output_path, gaussians_, format, scale_multiplier_);
            return true;
        } catch (const std::exception& e) {
            py::print("Error saving to PLY: ", e.what());
            return false;
        }
    }
    
    // Get gaussian count
    size_t get_gaussian_count() const {
        return gaussians_.size();
    }
    
    // Get gaussian data as numpy arrays
    py::tuple get_gaussian_data() {
        if (gaussians_.empty()) {
            return py::make_tuple(
                py::array_t<float>(std::vector<py::ssize_t>{0, 3}),
                py::array_t<float>(std::vector<py::ssize_t>{0, 4}),
                py::array_t<float>(std::vector<py::ssize_t>{0, 3}),
                py::array_t<float>(std::vector<py::ssize_t>{0, 4}),
                py::array_t<float>(std::vector<py::ssize_t>{0, 4})
            );
        }
        
        size_t n = gaussians_.size();
        
        // Create numpy arrays
        std::vector<py::ssize_t> shape = {static_cast<py::ssize_t>(n), 3};
        auto positions = py::array_t<float>(shape);
        shape = {static_cast<py::ssize_t>(n), 4};
        auto colors = py::array_t<float>(shape);
        shape = {static_cast<py::ssize_t>(n), 3};
        auto scales = py::array_t<float>(shape);
        shape = {static_cast<py::ssize_t>(n), 4};
        auto rotations = py::array_t<float>(shape);
        shape = {static_cast<py::ssize_t>(n), 3};
        auto normals = py::array_t<float>(shape);
        
        auto positions_buf = positions.request();
        auto colors_buf = colors.request();
        auto scales_buf = scales.request();
        auto rotations_buf = rotations.request();
        auto normals_buf = normals.request();
        
        float* positions_ptr = static_cast<float*>(positions_buf.ptr);
        float* colors_ptr = static_cast<float*>(colors_buf.ptr);
        float* scales_ptr = static_cast<float*>(scales_buf.ptr);
        float* rotations_ptr = static_cast<float*>(rotations_buf.ptr);
        float* normals_ptr = static_cast<float*>(normals_buf.ptr);
        
        for (size_t i = 0; i < n; ++i) {
            const auto& g = gaussians_[i];
            
            // Position (x, y, z)
            positions_ptr[i * 3 + 0] = g.position.x;
            positions_ptr[i * 3 + 1] = g.position.y;
            positions_ptr[i * 3 + 2] = g.position.z;
            
            // Color (r, g, b, a)
            colors_ptr[i * 4 + 0] = g.color.x;
            colors_ptr[i * 4 + 1] = g.color.y;
            colors_ptr[i * 4 + 2] = g.color.z;
            colors_ptr[i * 4 + 3] = g.color.w;
            
            // Scale (x, y, z)
            scales_ptr[i * 3 + 0] = g.scale.x;
            scales_ptr[i * 3 + 1] = g.scale.y;
            scales_ptr[i * 3 + 2] = g.scale.z;
            
            // Rotation (w, x, y, z) - quaternion
            rotations_ptr[i * 4 + 0] = g.rotation.w;
            rotations_ptr[i * 4 + 1] = g.rotation.x;
            rotations_ptr[i * 4 + 2] = g.rotation.y;
            rotations_ptr[i * 4 + 3] = g.rotation.z;
            
            // Normal (x, y, z)
            normals_ptr[i * 3 + 0] = g.normal.x;
            normals_ptr[i * 3 + 1] = g.normal.y;
            normals_ptr[i * 3 + 2] = g.normal.z;
        }
        
        return py::make_tuple(positions, colors, scales, rotations, normals);
    }
    
    // Set scale multiplier
    void set_scale_multiplier(float multiplier) {
        scale_multiplier_ = multiplier;
    }
    
    // Get scale multiplier
    float get_scale_multiplier() const {
        return scale_multiplier_;
    }
};

PYBIND11_MODULE(mesh2splat_core, m) {
    m.doc() = "Python bindings for Mesh2Splat - Fast mesh to 3D Gaussian splat conversion";
    
    // Expose the main converter class
    py::class_<Mesh2SplatConverter>(m, "Mesh2SplatConverter")
        .def(py::init<>())
        .def("convert_glb_to_gaussians", &Mesh2SplatConverter::convert_glb_to_gaussians, 
             py::arg("glb_path"), py::arg("sampling_density") = 1.0f,
             "Convert a GLB mesh file to Gaussian splats")
        .def("save_to_ply", &Mesh2SplatConverter::save_to_ply,
             py::arg("output_path"), py::arg("format") = 0,
             "Save the converted gaussians to a PLY file")
        .def("get_gaussian_count", &Mesh2SplatConverter::get_gaussian_count,
             "Get the number of generated gaussians")
        .def("get_gaussian_data", &Mesh2SplatConverter::get_gaussian_data,
             "Get gaussian data as numpy arrays (positions, colors, scales, rotations, normals)")
        .def("set_scale_multiplier", &Mesh2SplatConverter::set_scale_multiplier,
             py::arg("multiplier"),
             "Set the scale multiplier for gaussian sizes")
        .def("get_scale_multiplier", &Mesh2SplatConverter::get_scale_multiplier,
             "Get the current scale multiplier")
        .def_property("scale_multiplier", 
                     &Mesh2SplatConverter::get_scale_multiplier,
                     &Mesh2SplatConverter::set_scale_multiplier,
                     "Scale multiplier for gaussian sizes");
    
    // Expose constants
    m.attr("PLY_FORMAT_STANDARD") = 0;
    m.attr("PLY_FORMAT_PBR") = 1;
    m.attr("PLY_FORMAT_COMPRESSED_PBR") = 2;
    
    // Expose utility functions
    m.def("get_file_extension", &utils::getFileExtension,
           py::arg("filename"),
           "Get the file extension of a filename");
    
    m.def("triangle_area", &utils::triangleArea,
           py::arg("A"), py::arg("B"), py::arg("C"),
           "Calculate the area of a triangle defined by three 3D points");
}
