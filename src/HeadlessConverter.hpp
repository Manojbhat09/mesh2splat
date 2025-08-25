#pragma once

#include <string>

class HeadlessConverter {
public:
    HeadlessConverter();
    ~HeadlessConverter();

    bool convert(const std::string& glb_path, const std::string& output_ply_path, float sampling_density = 1.0f, unsigned int ply_format = 0);

private:
    struct GLFWwindow* window;
};
