# Used to convert a JPG to a compressed JPG wihtout progressive encoding
# These JPG's can be stored in Inkplate's memory

from PIL import Image
import os

def compress_images_in_folder():
    # Get the current working directory
    current_folder = os.getcwd()

    # Iterate through all files in the folder
    for filename in os.listdir(current_folder):
        # Process only .jpg and .jpeg files (case-insensitive)
        if filename.lower().endswith(('.jpg', '.jpeg')):
            try:
                # Open the image
                with Image.open(filename) as img:
                    # Convert to RGB (ensures compatibility with baseline JPEG)
                    img = img.convert("RGB")

                    # Construct the new filename with '_min' appended
                    name, ext = os.path.splitext(filename)
                    new_filename = f"{name}_min{ext}"

                    # Save the image with baseline compression
                    img.save(new_filename, "JPEG", quality=85, optimize=True, progressive=False)

                    print(f"Compressed and saved: {new_filename}")

            except Exception as e:
                print(f"Error processing {filename}: {e}")

if __name__ == "__main__":
    compress_images_in_folder()
