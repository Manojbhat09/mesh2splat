from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext
import pybind11
import os

# Get the directory containing this setup.py
current_dir = os.path.dirname(os.path.abspath(__file__))

# Define the extension module
ext_modules = [
    Pybind11Extension(
        "mesh2splat_core",
        sources=[
            "src/pybind11_bindings.cpp",
            "src/parsers/parsers.cpp",
            "src/utils/utils_simplified.cpp",
            "src/implementationFiles/stb_image_impl.cpp",
            "src/implementationFiles/stb_image_resize_impl.cpp",
            "src/implementationFiles/stb_image_write_impl.cpp",
            "src/implementationFiles/tiny_gltf_impl.cpp",
        ],
        include_dirs=[
            pybind11.get_include(),
            "src",
            "thirdParty/glm",
            "thirdParty",
        ],
        libraries=[],
        language="c++",
        cxx_std=17,
        extra_compile_args=["-fPIC", "-O3"],
        extra_link_args=["-fPIC"],
    ),
]

setup(
    name="mesh2splat",
    version="0.1.0",
    author="Stefano Scolari",
    author_email="stefano.scolari@ea.com",
    description="Fast mesh to 3D Gaussian splat conversion",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.8",
    install_requires=[
        "pybind11>=2.6.0",
        "numpy>=1.19.0",
        "trimesh>=3.9.0",
    ],
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Multimedia :: Graphics :: 3D Rendering",
        "Topic :: Scientific/Engineering :: Visualization",
    ],
)
