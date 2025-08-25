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
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#define EMPTY_TEXTURE "empty_texture"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>
#include "params.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


static void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        abort();
    }
}

#ifdef _DEBUG
    #define GL_CHECK(stmt) do { \
            stmt; \
            CheckOpenGLError(#stmt, __FILE__, __LINE__); \
        } while (0)
#else
    #define GL_CHECK(stmt) stmt
#endif

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

    struct MaterialGltf {
        std::string name;
        glm::vec4 baseColorFactor;              // Name of material, default is white
        TextureInfo baseColorTexture;           // Texture for the base color
        TextureInfo normalTexture;              // Normal map
        TextureInfo metallicRoughnessTexture;   // Contains the metalness value in the "blue" color channel, and the roughness value in the "green" color channel
        TextureInfo occlusionTexture;           // Texture for occlusion mapping
        TextureInfo emissiveTexture;            // Texture for emissive mapping
        float metallicFactor;                   // Metallic-Roughness map
        float roughnessFactor;                  // Metallic-Roughness map
        float occlusionStrength;                // Strength of occlusion effect
        float normalScale;                      // Scale of normal map
        glm::vec3 emissiveFactor;               // Emissive color factor

        MaterialGltf() : name("Default"), baseColorFactor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
            baseColorTexture(TextureInfo()), normalTexture(TextureInfo()), metallicRoughnessTexture(TextureInfo()),
            occlusionTexture(TextureInfo()), emissiveTexture(TextureInfo()),
            metallicFactor(1.0f), roughnessFactor(1.0f), occlusionStrength(1.0f), normalScale(1.0f), emissiveFactor(glm::vec3(1.0f, 1.0f, 1.0f)) {}

        MaterialGltf(const std::string& name, const glm::vec4& baseColorFactor) :
            name(name), baseColorFactor(baseColorFactor),
            baseColorTexture(TextureInfo()), normalTexture(TextureInfo()), metallicRoughnessTexture(TextureInfo()),
            occlusionTexture(TextureInfo()), emissiveTexture(TextureInfo()),
            metallicFactor(1.0f), roughnessFactor(1.0f), occlusionStrength(1.0f), normalScale(1.0f), emissiveFactor(glm::vec3(1.0f, 1.0f, 1.0f)) {}

        MaterialGltf(const std::string& name, const glm::vec4& baseColorFactor, const TextureInfo& baseColorTexture,
            const TextureInfo& normalTexture, const TextureInfo& metallicRoughnessTexture, const TextureInfo& occlusionTexture,
            const TextureInfo& emissiveTexture, float metallicFactor, float roughnessFactor, float occlusionStrength, float normalScale,
            glm::vec3 emissiveFactor) : name(name), baseColorFactor(baseColorFactor), baseColorTexture(baseColorTexture),
            normalTexture(normalTexture), metallicRoughnessTexture(metallicRoughnessTexture), occlusionTexture(occlusionTexture),
            emissiveTexture(emissiveTexture), metallicFactor(metallicFactor), roughnessFactor(roughnessFactor), occlusionStrength(occlusionStrength),
            normalScale(normalScale), emissiveFactor(emissiveFactor) {}
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

    struct GLMesh {
        GLuint vao;
        GLuint vbo;
        size_t vertexCount;
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

    enum ModelFileExtension
    {
        NONE,
        PLY,
        GLB,
    };

    bool pointInTriangle(const glm::vec2& pt, const glm::vec2& v1, const glm::vec2& v2, const glm::vec2& v3);

    bool computeBarycentricCoords(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, float& u, float& v, float& w);

    glm::vec3 getShFromColor(glm::vec3 color);

    glm::vec3 getColorFromSh(glm::vec3 sh);

    glm::vec3 floatToVec3(const float val);

    int inline sign(float x);

    glm::vec2 pixelToUV(const glm::ivec2& pixel, int textureWidth, int textureHeight);

    glm::ivec2 uvToPixel(const glm::vec2& uv, int textureWidth, int textureHeight);

    std::pair<glm::vec2, glm::vec2> computeUVBoundingBox(const glm::vec2* triangleUVs);

    //https://www.nayuki.io/res/srgb-transform-library/srgb-transform.c
    float linear_to_srgb_float(float x); //Assumes 0,...,1 range 

    glm::vec3 linear_to_srgb_float(glm::vec3 rgb);

    //https://www.nayuki.io/res/srgb-transform-library/srgb-transform.c
    float srgb_to_linear_float(float x); //Assumes 0,...,1 range 

    glm::vec3 srgb_to_linear_float(glm::vec3 rgb);

    glm::vec4 rgbaAtPos(const int width, int X, int Y, unsigned char* rgb_image, const int bpp);

    float displacementAtPos(const int width, int X, int Y, unsigned char* displacement_image);

    float computeTriangleAreaUV(const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3);

    void convert_xyz_to_cube_uv(float x, float y, float z, int* index, float* u, float* v);

    void convert_cube_uv_to_xyz(int index, float u, float v, float* x, float* y, float* z);

    void computeAndLoadTextureInformation(
        std::map<std::string, std::pair<unsigned char*, int>>& textureTypeMap,
        MaterialGltf& material, const int x, const int y,
        glm::vec4& rgba,
        float& metallicFactor, float& roughnessFactor,
        glm::vec3& interpolatedNormal, glm::vec3& outputNormal, glm::vec4& interpolatedTangent
    );

    bool shouldSkip(const GaussianDataSSBO& g);

    inline float sigmoid(float opacity) { return 1.0 / (1.0 + std::exp(-opacity)); };

    std::string formatWithCommas(int value);

    ModelFileExtension getFileExtension(const std::string& filename);

    float triangleArea(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C);

    std::string getExecutablePath();

    std::string getExecutableDir();

    fs::path relative(fs::path p, fs::path base);
}