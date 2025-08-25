///////////////////////////////////////////////////////////////////////////////
//         Mesh2Splat: fast mesh to 3D gaussian splat conversion             //
//        Copyright (c) 2025 Electronic Arts Inc. All rights reserved.       //
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32
    #define _CRTDBG_MAP_ALLOC  
    #include <crtdbg.h>  
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    // Linux/Unix includes
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

#include <stdlib.h>  
#include <string>
#include <vector>
#include <deque>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <thread>
#include <map>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <locale>
#include <sstream>
#include <limits>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#define EMPTY_TEXTURE "empty_texture"
#define MAX_RESOLUTION_TARGET 2048
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>

namespace fs = std::experimental::filesystem;

namespace utils
{
    struct Material {
        glm::vec3 ambient;       // Ka
        glm::vec3 diffuse;       // Kd
        glm::vec3 specular;      // Ks
        float specularExponent;  // Ns
        float transparency;      // d or Tr
        float opticalDensity;    // Ni
        std::string diffuseMap;  // map_Kd, texture map

        Material() : ambient(0.0f), diffuse(0.0f), specular(0.0f), specularExponent(0.0f), transparency(1.0f), opticalDensity(1.0f) {}
    };

    struct TextureInfo {
        std::string path;
        int texCoordIndex; // Texture coordinate set index used by this texture
        std::vector<unsigned char> texture;
        int width, height;
        unsigned int channels;

        TextureInfo(const std::string& path = EMPTY_TEXTURE, int texCoordIndex = 0, std::vector<unsigned char> texture = {}, int width = 0, int height = 0, unsigned int channels = 0) : path(path), texCoordIndex(texCoordIndex), texture(texture), width(width), height(height), channels(channels) {}
    };

    struct TextureDataGl {
        std::vector<unsigned char> textureData;
        unsigned int channels       = 0;
        unsigned int glTextureID    = 0;
        unsigned int width          = 0;
        unsigned int height         = 0;

        TextureDataGl(std::vector<unsigned char> textureData, unsigned int channels, unsigned int glTextureID, unsigned int width, unsigned int height) : textureData(textureData), channels(channels), glTextureID(glTextureID), width(width), height(height){}
        
        TextureDataGl(std::vector<unsigned char> textureData, unsigned int channels) : textureData(textureData), channels(channels), glTextureID(0){}
        
        TextureDataGl(TextureInfo info)
        {
            textureData = info.texture;
            channels = info.channels;
            glTextureID = 0;
            width = info.width;
            height = info.height;
        }
    };

    struct MaterialGltf {
        std::string name;
        glm::vec4 baseColorFactor;              // Name of material, default is white
        TextureInfo baseColorTexture;           // Texture for the base color
        TextureInfo normalTexture;              // Normal map
        TextureInfo metallicRoughnessTexture;   // Metallic-roughness texture
        float metallicFactor;                   // Metallic factor
        float roughnessFactor;                  // Roughness factor
        glm::vec3 emissiveFactor;              // Emissive factor
        float alphaCutoff;                      // Alpha cutoff value
        bool doubleSided;                       // Double sided flag
        std::string alphaMode;                  // Alpha mode (OPAQUE, MASK, BLEND)
        bool unlit;                             // Unlit flag

        MaterialGltf() : name("Default"), baseColorFactor(1.0f), metallicFactor(1.0f), roughnessFactor(1.0f), emissiveFactor(0.0f), alphaCutoff(0.5f), doubleSided(false), alphaMode("OPAQUE"), unlit(false) {}
    };

    struct Gaussian3D {
        Gaussian3D(glm::vec3 position, glm::vec3 normal, glm::vec3 scale, glm::vec4 rotation, glm::vec3 RGB, float opacity, MaterialGltf material)
            : position(position), normal(normal), scale(scale), rotation(rotation), sh0(RGB), opacity(opacity), material(material) {};
        Gaussian3D() : position(0.0f), normal(0.0f), scale(0.0f), rotation(0.0f), sh0(0.0f), opacity(0.0f), material(MaterialGltf()) {};
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 scale;
        glm::vec4 rotation;
        glm::vec3 sh0;
        float opacity;
        MaterialGltf material;
    };

    struct GaussianDataSSBO {
        glm::vec4 position;
        glm::vec4 color;
        glm::vec4 scale;
        glm::vec4 normal;
        glm::vec4 rotation;
        glm::vec4 pbr;
    };

    struct Face {
        glm::vec3 pos[3];
        glm::vec2 uv[3];
        glm::vec2 normalizedUvs[3]; //Resulting from xatlas
        glm::vec3 normal[3];
        glm::vec4 tangent[3];
        glm::vec3 scale;
        glm::vec4 rotation;
    };

    struct BBox
    {
        glm::vec3 min;
        glm::vec3 max;

        BBox(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}
    };

    struct Mesh {
        std::string name;
        std::vector<Face> faces; // Tuple of vertex indices, uv indices and normalIndices
        MaterialGltf material; 
        float surfaceArea = 0;
        BBox bbox = BBox(glm::vec3(0), glm::vec3(0));

        Mesh(const std::string& name = "Unnamed") : name(name) {}
    };

    enum ModelFileExtension
    {
        NONE,
        PLY,
        GLB,
    };

    // Utility functions - only the ones actually needed for conversion
    glm::vec3 getShFromColor(glm::vec3 color);
    glm::vec3 getColorFromSh(glm::vec3 sh);
    inline float sigmoid(float opacity) { return 1.0 / (1.0 + std::exp(-opacity)); };
    std::string getFileExtension(const std::string& filename);
    float triangleArea(const std::vector<float>& A, const std::vector<float>& B, const std::vector<float>& C);
}
