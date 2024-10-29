def file_to_hex():
    # Read the file in binary mode
    filename = "parrotSolderedBmp.Bmp"
    with open(filename, 'rb') as f:
        file_data = f.read()

    # Convert file data to a list of hex values
    hex_values = [f'0x{byte:02X}' for byte in file_data]

    # Split hex values into rows of 100 bytes
    rows = [', '.join(hex_values[i:i + 100]) for i in range(0, len(hex_values), 100)]
    hex_string = ',\n    '.join(rows)  # Add indentation for each new row

    # Format the output
    output = f'const uint8_t {filename.split(".")[0]}[] = \n{{\n    {hex_string},\n}};'

    # Save to a .h file
    output_filename = f'{filename.split(".")[0]}.h'
    with open(output_filename, 'w') as output_file:
        output_file.write(output)

    print(f'Hex data saved to {output_filename}')

# Call the function to execute it
file_to_hex()
