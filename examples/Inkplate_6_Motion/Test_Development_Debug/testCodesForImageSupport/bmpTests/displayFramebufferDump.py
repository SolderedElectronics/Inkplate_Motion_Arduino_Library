from PIL import Image

# File path to your hex data file
file_path = 'framebufferDump.txt'

# Image dimensions
width = 1024
height = 758

# Read hex RGB888 data from the file
with open(file_path, 'r') as f:
    # Split the file content into individual hex values
    hex_values = f.read().split()

# Ensure that the file contains the expected number of pixels (width * height)
expected_pixel_count = width * height
if len(hex_values) != expected_pixel_count:
    raise ValueError(f"Expected {expected_pixel_count} pixels but got {len(hex_values)}")

# Convert hex strings to RGB tuples
rgb_data = [(int(hex_val[0:2], 16), int(hex_val[2:4], 16), int(hex_val[4:6], 16)) for hex_val in hex_values]

# Create the image
img = Image.new('RGB', (width, height))
img.putdata(rgb_data)

# Display the image
img.show()

# Optionally, save the image to a file
img.save('output_image.png')
