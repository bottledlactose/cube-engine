import os
import subprocess
import sys

# Path to the glslc compiler
GLSLC = "glslc"

def main():
    if len(sys.argv) != 3:
        print("Usage: python compile_shaders.py <shaders_directory> <output_directory>")
        sys.exit(1)

    shaders_dir = sys.argv[1]
    output_dir = sys.argv[2]

    if not os.path.exists(shaders_dir):
        print("Error: shaders directory not found")
        sys.exit(1)
    
    if not os.path.exists(output_dir):
        os.makedirs(output_dir, exist_ok=True)
    
    for filename in os.listdir(shaders_dir):
        if filename.endswith(".vert") or filename.endswith(".frag"):
            input_path = os.path.join(shaders_dir, filename)
            output_path = os.path.join(output_dir, filename + ".spv")

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

            header_path = os.path.join(output_dir, filename + ".h")
            with open(header_path, "w") as f:
                f.write("unsigned char " + variable_name + "[] = {\n")
                f.write(", ".join(f"0x{byte:02x}" for byte in spv_data))
                f.write("\n};\n")
                f.write(f"unsigned int " + variable_name + "_SIZE = sizeof(" + variable_name + ");\n")

if __name__ == "__main__":
    main()
