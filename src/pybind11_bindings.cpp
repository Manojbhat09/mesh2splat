#include <pybind11/pybind11.h>
#include "HeadlessConverter.hpp"

namespace py = pybind11;

PYBIND11_MODULE(mesh2splat_core, m) {
    m.doc() = "Python bindings for Mesh2Splat - Fast mesh to 3D Gaussian splat conversion";

    m.def("convert", [](const std::string& glb_path, const std::string& output_ply_path, float sampling_density, unsigned int ply_format) {
        HeadlessConverter converter;
        return converter.convert(glb_path, output_ply_path, sampling_density, ply_format);
    }, py::arg("glb_path"), py::arg("output_ply_path"), py::arg("sampling_density") = 1.0f, py::arg("ply_format") = 0, "Convert a GLB mesh file to a PLY file of Gaussian splats.");

    m.attr("PLY_FORMAT_STANDARD") = 0;
    m.attr("PLY_FORMAT_PBR") = 1;
    m.attr("PLY_FORMAT_COMPRESSED_PBR") = 2;
}
