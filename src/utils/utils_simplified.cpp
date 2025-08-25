///////////////////////////////////////////////////////////////////////////////
//         Mesh2Splat: fast mesh to 3D gaussian splat conversion             //
//        Copyright (c) 2025 Electronic Arts Inc. All rights reserved.       //
///////////////////////////////////////////////////////////////////////////////

#include "utils_simplified.hpp"
#include <cmath>
#include <algorithm>

namespace utils
{
    glm::vec3 getShFromColor(glm::vec3 color) {
        // Simple spherical harmonics conversion - just return the color for now
        // In a full implementation, this would convert to spherical harmonics coefficients
        return color;
    }

    glm::vec3 getColorFromSh(glm::vec3 sh) {
        // Simple spherical harmonics conversion - just return the SH coefficients for now
        // In a full implementation, this would convert from spherical harmonics coefficients
        return sh;
    }

    std::string getFileExtension(const std::string& filename) {
        if (filename.empty()) return "none";
        
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) return "none";
        
        std::string extension = filename.substr(dotPos + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (extension == "ply") return "ply";
        if (extension == "glb") return "glb";
        
        return "none";
    }

    float triangleArea(const std::vector<float>& A, const std::vector<float>& B, const std::vector<float>& C) {
        if (A.size() < 3 || B.size() < 3 || C.size() < 3) {
            return 0.0f;
        }
        
        glm::vec3 vecA(A[0], A[1], A[2]);
        glm::vec3 vecB(B[0], B[1], B[2]);
        glm::vec3 vecC(C[0], C[1], C[2]);
        
        glm::vec3 AB = vecB - vecA;
        glm::vec3 AC = vecC - vecA;
        glm::vec3 cross_product = glm::cross(AB, AC);
        return 0.5f * glm::length(cross_product);
    }
}
