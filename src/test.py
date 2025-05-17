import struct
import zlib
import png

def create_single_red_pixel_png(filename="red_pixel.png"):
    """
    Creates a PNG file with a single red pixel (255, 0, 0)
    """
    # PNG file signature
    signature = b'\x89PNG\r\n\x1a\n'
    
    # IHDR chunk (Image header)
    width = 1
    height = 1
    bit_depth = 8
    color_type = 2  # RGB
    compression = 0
    filter_method = 0
    interlace = 0
    
    ihdr_data = struct.pack(">IIBBBBB", 
                          width, height, 
                          bit_depth, color_type, 
                          compression, filter_method, interlace)
    ihdr_chunk = create_chunk(b'IHDR', ihdr_data)
    
    # IDAT chunk (Image data)
    # For a single RGB pixel, we need:
    # - 1 byte for filter type (0)
    # - 3 bytes for RGB values (255, 0, 0)
    raw_data = b'\x00\xff\x00\x00'  # Filter type 0 + RGB values
    
    # Compress the data using zlib
    compressor = zlib.compressobj(level=1, strategy=zlib.Z_FIXED)
    compressed_data = compressor.compress(raw_data)
    compressed_data += compressor.flush() # Ensure all compressed data is written
    idat_chunk = create_chunk(b'IDAT', compressed_data)
    
    # IEND chunk (End of image)
    iend_chunk = create_chunk(b'IEND', b'')
    
    # Write to file
    with open(filename, 'wb') as f:
        f.write(signature)
        f.write(ihdr_chunk)
        f.write(idat_chunk)
        f.write(iend_chunk)
    
    print(f"Created PNG file '{filename}' with a single red pixel")
    
    # Optionally, print the hex dump of the file
    print_hex_dump(filename)

def create_chunk(chunk_type, data):
    """
    Creates a PNG chunk with the given type and data.
    Returns the complete chunk including length, type, data, and CRC.
    """
    # Calculate the CRC
    crc = zlib.crc32(chunk_type)
    crc = zlib.crc32(data, crc) & 0xffffffff
    
    # Create the chunk
    chunk = struct.pack(">I", len(data))  # Length
    chunk += chunk_type                   # Type
    chunk += data                         # Data
    chunk += struct.pack(">I", crc)       # CRC
    
    return chunk

def print_hex_dump(filename):
    """
    Prints a hex dump of the given file.
    """
    with open(filename, 'rb') as f:
        data = f.read()
    
    print("Hex dump of the created PNG file:")
    
    for i in range(0, len(data), 16):
        line = data[i:i+16]
        # Print address
        print(f"{i:08X} | ", end="")
        
        # Print hex values
        hex_values = " ".join(f"{byte:02X}" for byte in line)
        # Pad hex values to maintain alignment
        print(f"{hex_values:<47} | ", end="")
        
        # Print ASCII representation
        ascii_rep = "".join(chr(byte) if 32 <= byte <= 126 else "." for byte in line)
        print(f"{ascii_rep}")
    
    print(f"File size: {len(data)} bytes")

if __name__ == "__main__":
    create_single_red_pixel_png()

# Alternative implementation using the PyPNG library, which is easier but less educational
def create_single_red_pixel_png_with_pypng(filename="red_pixel_pypng.png"):
    """
    Creates a PNG file with a single red pixel (255, 0, 0) using the PyPNG library
    """
    # RGB values for a single red pixel
    pixel = [(255, 0, 0)]
    
    # Create a PNG writer for a 1x1 image with RGB values
    writer = png.Writer(width=1, height=1, greyscale=False, alpha=False)
    
    # Write the PNG file
    with open(filename, 'wb') as f:
        writer.write(f, [pixel[0]])
    
    print(f"Created PNG file '{filename}' with a single red pixel using PyPNG")
    print_hex_dump(filename)

# If you want to use the PyPNG version instead, uncomment the following line:
# create_single_red_pixel_png_with_pypng()