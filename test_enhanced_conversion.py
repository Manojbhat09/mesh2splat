#!/usr/bin/env python3
"""
Test script for enhanced GLB to Gaussian conversion
"""

import mesh2splat_core
import os

def test_enhanced_conversion():
    print("Testing enhanced GLB to Gaussian conversion...")
    
    # Initialize converter
    converter = mesh2splat_core.Mesh2SplatConverter()
    
    # Set scale multiplier
    converter.set_scale_multiplier(3.0)
    print(f"Scale multiplier set to: {converter.get_scale_multiplier()}")
    
    # GLB file path
    glb_path = "/home/mbhat/three-gen-subnet-trellis/wbgmsst,_a_blue_monkey_sitting_on_temple,_3d_isometric,_white_background.glb"
    
    if not os.path.exists(glb_path):
        print(f"Error: GLB file not found: {glb_path}")
        return False
    
    print(f"Converting GLB file: {glb_path}")
    
    # Convert with density 5.0
    success = converter.convert_glb_to_gaussians(glb_path, 5.0)
    
    print(f"Conversion success: {success}")
    if success:
        print(f"Gaussian count: {converter.get_gaussian_count()}")
        
        # Save to PLY
        output_path = "/home/mbhat/three-gen-subnet-trellis/enhanced_monkey_gaussians.ply"
        converter.save_to_ply(output_path, 1)  # PBR format
        print(f"Saved to: {output_path}")
        
        # Get gaussian data for inspection
        positions, colors, scales, rotations, normals = converter.get_gaussian_data()
        print(f"Data shapes:")
        print(f"  Positions: {positions.shape}")
        print(f"  Colors: {colors.shape}")
        print(f"  Scales: {scales.shape}")
        print(f"  Rotations: {rotations.shape}")
        print(f"  Normals: {normals.shape}")
        
        # Show some sample data
        if len(positions) > 0:
            print(f"\nFirst gaussian:")
            print(f"  Position: {positions[0]}")
            print(f"  Color: {colors[0]}")
            print(f"  Scale: {scales[0]}")
            print(f"  Rotation: {rotations[0]}")
            print(f"  Normal: {normals[0]}")
        
        return True
    else:
        print("Conversion failed!")
        return False

if __name__ == "__main__":
    test_enhanced_conversion()
