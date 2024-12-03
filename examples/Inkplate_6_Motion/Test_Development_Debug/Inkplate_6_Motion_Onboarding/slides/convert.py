import os

def jpg_to_hex_header(jpg_filename):
    # Ensure the input file is in the same directory as the script
    script_dir = os.getcwd()
    jpg_path = os.path.join(script_dir, jpg_filename)

    # Check if the input file exists
    if not os.path.exists(jpg_path):
        print(f"Error: File '{jpg_path}' does not exist.")
        return

    # Define the output header file path
    output_filename = os.path.splitext(jpg_filename)[0] + ".h"
    output_path = os.path.join(script_dir, output_filename)

    try:
        # Read the binary content of the file
        with open(jpg_path, "rb") as f:
            binary_data = f.read()

        # Convert binary data to a comma-separated string of hex values
        hex_values = [f"0x{byte:02x}" for byte in binary_data]
        formatted_hex_data = ", \n".join(
            ", ".join(hex_values[i:i+40]) for i in range(0, len(hex_values), 40)
        )

        # Create the header file content
        header_content = (
            f"#ifndef {os.path.splitext(jpg_filename)[0].upper()}_H\n"
            f"#define {os.path.splitext(jpg_filename)[0].upper()}_H\n\n"
            f"#include <stdint.h>\n\n"
            f"const uint8_t {os.path.splitext(jpg_filename)[0]}[] = {{\n{formatted_hex_data}\n}};\n\n"
            f"#endif // {os.path.splitext(jpg_filename)[0].upper()}_H"
        )

        # Write the content to the output header file
        with open(output_path, "w") as header_file:
            header_file.write(header_content)

        print(f"Header file '{output_filename}' has been created successfully in the same folder as the script.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    script_dir = os.getcwd()

    # Get all .jpg files in the current directory
    jpg_files = [f for f in os.listdir(script_dir) if f.lower().endswith(".jpg")]

    if not jpg_files:
        print("No .jpg files found in the current folder.")
    else:
        print(f"Found {len(jpg_files)} .jpg file(s). Converting...")

        for jpg_filename in jpg_files:
            print(f"Processing: {jpg_filename}")
            jpg_to_hex_header(jpg_filename)

        print("All files have been processed.")
