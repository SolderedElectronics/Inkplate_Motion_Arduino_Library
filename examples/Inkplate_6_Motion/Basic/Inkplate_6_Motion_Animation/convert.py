import os
from PIL import Image

def dither_image(input_path, output_path, index):
    # Open image
    img = Image.open(input_path)

    # Convert to grayscale
    img = img.convert("L")

    # Apply Floyd-Steinberg dithering
    img_dithered = img.convert("1", dither=Image.FLOYDSTEINBERG)

    # Get image size (width and height).
    width, height = img.size

    # Save dithered image
    # img_dithered.save(output_path)

    # Create header file.
    header_file = open(output_path, 'w')
    header_guard = "__FRAME" + str(index).zfill(3) + "_H__"
    header_file.write("#ifndef " + header_guard + "\n")
    header_file.write("#define " + header_guard + "\n")
    header_file.write("const uint8_t frame" + str(index).zfill(3) + "[] PROGMEM = \n")
    header_file.write("{\n")

    for y in range(0, height):
        for x in range(0, width, 8):
            dataByte = 0
            for b in range(0, 8):
                    if (img_dithered.getpixel((x + b, y)) != 255):
                         dataByte = dataByte | (1 << (7 - b))
            header_file.write(hex(dataByte).zfill(2) + ", ")
        header_file.write("\n")

    header_file.write("};\r\n#endif")
    header_file.close()


def batch_dither_images(root_folder):
    # Create 'dithered' subfolder if it doesn't exist
    dithered_folder = os.path.join(root_folder, 'dithered')
    if not os.path.exists(dithered_folder):
        os.makedirs(dithered_folder)

    # Get list of JPEG files in root folder
    jpeg_files = [f for f in os.listdir(root_folder) if f.lower().endswith('.jpg')]

    index = 0
    # Dither each JPEG image and save to 'dithered' subfolder
    for filename in jpeg_files:

        input_path = os.path.join(root_folder, filename)
        output_path = os.path.join(dithered_folder, "frame" + str(index) + ".h")
        dither_image(input_path, output_path, index)
        index = index + 1

if __name__ == "__main__":
    # Specify root folder containing JPEG images
    root_folder = '.'  # Change this to your desired root folder

    # Batch dither images
    batch_dither_images(root_folder)
