import os
import subprocess
import sys

# Path to the glslc compiler
GLSLC = "glslc"
# Path to the shaders directory
SHADERS_DIR = "shaders"
# Path to the SPIR-V output directory
OUTPUT_DIR = "build/Debug/shaders"

# Check if shaders dir exists
if not os.path.exists(SHADERS_DIR):
    print("Error: shaders directory not found")
    sys.exit(1)

# Create the output directory if it doesn't exist
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# Dynamically loop through all files in the shaders directory
for filename in os.listdir(SHADERS_DIR):
    if filename.endswith(".vert") or filename.endswith(".frag"):
        input_path = os.path.join(SHADERS_DIR, filename)
        output_path = os.path.join(OUTPUT_DIR, filename + ".spv")
        print("Compiling", input_path, "to", output_path)
        subprocess.run([GLSLC, input_path, "-o", output_path], check=True)
