#!/usr/bin/env python3
"""
Test script for enhanced GLB to Gaussian conversion
"""

import mesh2splat_core
import os

def test_enhanced_conversion():
    print("Testing enhanced GLB to Gaussian conversion...")
    
    # GLB file path
    glb_path = "/home/mbhat/three-gen-subnet-trellis/wbgmsst,_a_blue_monkey_sitting_on_temple,_3d_isometric,_white_background.glb"
    output_path = "/home/mbhat/three-gen-subnet-trellis/enhanced_monkey_gaussians.ply"
    
    if not os.path.exists(glb_path):
        print(f"Error: GLB file not found: {glb_path}")
        return False
    
    print(f"Converting GLB file: {glb_path}")
    
    # Convert with density 5.0 and PBR format
    success = mesh2splat_core.convert(
        glb_path,
        output_path,
        sampling_density=5.0,
        ply_format=mesh2splat_core.PLY_FORMAT_PBR
    )
    
    print(f"Conversion success: {success}")

    if success and os.path.exists(output_path):
        print(f"Output file created at: {output_path}")
        return True
    else:
        print("Conversion failed or output file not created.")
        return False

if __name__ == "__main__":
    test_enhanced_conversion()
