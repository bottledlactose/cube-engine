import os
import subprocess
import sys

# Path to the glslc compiler
GLSLC = "glslc"
# Path to the shaders directory
SHADERS_DIR = "shaders"

def main():
    if not os.path.exists(SHADERS_DIR):
        print("Error: shaders directory not found")
        sys.exit(1)
    
    for filename in os.listdir(SHADERS_DIR):
        if filename.endswith(".vert") or filename.endswith(".frag"):
            input_path = os.path.join(SHADERS_DIR, filename)
            output_path = os.path.join(SHADERS_DIR, filename + ".spv")

            # Ensure output path is deleted first
            if os.path.exists(output_path):
                os.remove(output_path)

            subprocess.run([GLSLC, input_path, "-o", output_path], check=False)

            # Check if output file was created
            if not os.path.exists(output_path):
                print("Error: output file was not created")
                sys.exit(1)

            # Generate variable name such as BASIC_TRIANGLE_SHADER_VERT
            variable_name = filename.upper().replace(".", "_") + "_SHADER"

            # Convert the compiled shader to a header file
            with open(output_path, "rb") as f:
                spv_data = f.read()

            header_path = os.path.join("source/graphics/" + SHADERS_DIR, filename + ".h")
            with open(header_path, "w") as f:
                f.write("unsigned char " + variable_name + "[] = {\n")
                f.write(", ".join(f"0x{byte:02x}" for byte in spv_data))
                f.write("\n};\n")
                f.write(f"unsigned int " + variable_name + "_LEN = sizeof(" + variable_name + ");\n")

if __name__ == "__main__":
    main()
