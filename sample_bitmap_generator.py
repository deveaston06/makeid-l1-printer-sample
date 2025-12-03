#!/usr/bin/env python3
import numpy as np
import minilzo
import math
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.patches import Rectangle

# === PRINTER SETTINGS ===
PRINTER_CONSTANT = bytes([0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01])
IMAGE_WIDTH = 255
IMAGE_HEIGHT = 96
CID_0004_HEADER_SIZE = 7

def calculate_checksum(frame_bytes):
    checksum = 0
    for byte in frame_bytes[:-1]:
        checksum = (checksum - byte) & 0xFF
    return checksum

def create_test_bitmap(width, height, pattern="border", text=None, text_x=10, text_y=10):
    bitmap = np.zeros((height, width), dtype=np.uint8)  # Start all white (0)

    if pattern == "border":
        border_size = 5
        bitmap[0:border_size, :] = 1                    # Top border
        bitmap[-border_size:, :] = 1                    # Bottom border  
        bitmap[:, 0:border_size] = 1                    # Left border
        bitmap[:, -border_size:] = 1                    # Right border

    elif pattern == "all_white":
        bitmap[:] = 0  # All white (0)
    elif pattern == "all_black":
        bitmap[:] = 1  # All black (1)
    elif pattern == "diagonal":
        for i in range(min(width, height)):
            bitmap[i, i] = 1  # Main diagonal 
            bitmap[i, width - 1 - i] = 1  # Anti-diagonal

    elif pattern == "stripes":
        stripe_width = 10
        for x in range(width):
            if (x // stripe_width) % 2 == 0:
                bitmap[:, x] = 1  # Black stripes

    elif pattern == "horizontal_stripes":
        stripe_height = 8
        for y in range(height):
            if (y // stripe_height) % 2 == 0:
                bitmap[y, :] = 1  # Black stripes

    elif pattern == "checkerboard":
        square_size = 10
        for y in range(height):
            for x in range(width):
                if ((x // square_size) + (y // square_size)) % 2 == 0:
                    bitmap[y, x] = 1  # Black squares
    
    elif pattern == "y_varying_text":
        # Each row gets its own number based on row index
        # Row 0: "111111...", Row 1: "222222...", etc.
        for y in range(0, height, text_y):  # ADD text_y SPACING HERE
            # Determine which character to use for this row
            row_num = (y // text_y % 10) + 1  # 1-10 repeating based on text_y spacing
            char_to_use = str(row_num)[-1]  # Use last digit if >9
            
            # Create bitmap for this single character
            char_bmp = create_letter_bitmap(char_to_use)
            char_height, char_width = char_bmp.shape
            
            # Repeat this character across the entire row with text_x spacing
            for start_x in range(0, width, char_width + text_x):
                # Copy character onto the row
                for char_y in range(char_height):
                    for char_x in range(char_width):
                        pixel_y = y + char_y
                        pixel_x = start_x + char_x
                        if (pixel_y < height and pixel_x < width and 
                            char_bmp[char_y, char_x] == 1):
                            bitmap[pixel_y, pixel_x] = 1

    elif pattern == "row_numbers":
        # Each row shows its row number in text
        for y in range(0, height, text_y):
            # Convert row index to text (e.g., "Row 1", "Row 2")
            row_text = f"{y}"
            text_bmp = create_text_bitmap(row_text)
            text_height, text_width = text_bmp.shape
            
            # Position text at the start of this row
            for char_y in range(text_height):
                for char_x in range(text_width):
                    pixel_y = y + char_y
                    pixel_x = char_x
                    if (pixel_y < height and pixel_x < width and 
                        text_bmp[char_y, char_x] == 1):
                        bitmap[pixel_y, pixel_x] = 1
    
    elif pattern == "text" and text:
        # Create text bitmap once
        text_bmp = create_text_bitmap(text)
        text_height, text_width = text_bmp.shape
        
        # Repeat text across the entire bitmap with specified spacing
        for start_y in range(0, height, text_y + text_height):
            for start_x in range(0, width, text_x + text_width):
                # Copy text onto the main bitmap at this position
                for y in range(text_height):
                    for x in range(text_width):
                        pixel_y = start_y + y
                        pixel_x = start_x + x
                        if (pixel_y < height and pixel_x < width and 
                            text_bmp[y, x] == 1):
                            bitmap[pixel_y, pixel_x] = 1  # Set pixel to black for text

    elif pattern == "letter" and text:
        # Create text bitmap once for the entire string
        text_bmp = create_text_bitmap(text)
        text_height, text_width = text_bmp.shape
        
        # Repeat the entire text string across the bitmap
        for start_y in range(0, height, text_y + text_height):
            for start_x in range(0, width, text_x + text_width):
                # Copy the entire text string onto the main bitmap at this position
                for y in range(text_height):
                    for x in range(text_width):
                        pixel_y = start_y + y
                        pixel_x = start_x + x
                        if (pixel_y < height and pixel_x < width and 
                            text_bmp[y, x] == 1):
                            bitmap[pixel_y, pixel_x] = 1  # Set pixel to black for text

    return bitmap

def create_letter_bitmap(letter):
    """Create a bitmap for a single letter"""
    # Simple 5x7 font for uppercase letters
    font = {
        'A': [0x0E, 0x11, 0x1F, 0x11, 0x11],
        'B': [0x1E, 0x11, 0x1E, 0x11, 0x1E],
        'C': [0x0E, 0x11, 0x10, 0x11, 0x0E],
        'D': [0x1E, 0x11, 0x11, 0x11, 0x1E],
        'E': [0x1F, 0x10, 0x1E, 0x10, 0x1F],
        'F': [0x1F, 0x10, 0x1E, 0x10, 0x10],
        'G': [0x0E, 0x11, 0x13, 0x11, 0x0F],
        'H': [0x11, 0x11, 0x1F, 0x11, 0x11],
        'I': [0x1F, 0x04, 0x04, 0x04, 0x1F],
        'J': [0x07, 0x02, 0x02, 0x12, 0x0C],
        'K': [0x11, 0x12, 0x1C, 0x12, 0x11],
        'L': [0x10, 0x10, 0x10, 0x10, 0x1F],
        'M': [0x11, 0x1B, 0x15, 0x11, 0x11],
        'N': [0x11, 0x19, 0x15, 0x13, 0x11],
        'O': [0x0E, 0x11, 0x11, 0x11, 0x0E],
        'P': [0x1E, 0x11, 0x1E, 0x10, 0x10],
        'Q': [0x0E, 0x11, 0x11, 0x15, 0x0F],
        'R': [0x1E, 0x11, 0x1E, 0x14, 0x13],
        'S': [0x0F, 0x10, 0x0E, 0x01, 0x1E],
        'T': [0x1F, 0x04, 0x04, 0x04, 0x04],
        'U': [0x11, 0x11, 0x11, 0x11, 0x0E],
        'V': [0x11, 0x11, 0x11, 0x0A, 0x04],
        'W': [0x11, 0x11, 0x15, 0x15, 0x0A],
        'X': [0x11, 0x0A, 0x04, 0x0A, 0x11],
        'Y': [0x11, 0x0A, 0x04, 0x04, 0x04],
        'Z': [0x1F, 0x02, 0x04, 0x08, 0x1F],
        '0': [0x0E, 0x11, 0x11, 0x11, 0x0E],
        '1': [0x04, 0x0C, 0x04, 0x04, 0x0E],
        '2': [0x0E, 0x11, 0x02, 0x04, 0x1F],
        '3': [0x0E, 0x11, 0x06, 0x11, 0x0E],
        '4': [0x02, 0x06, 0x0A, 0x1F, 0x02],
        '5': [0x1F, 0x10, 0x1E, 0x01, 0x1E],
        '6': [0x0E, 0x10, 0x1E, 0x11, 0x0E],
        '7': [0x1F, 0x02, 0x04, 0x08, 0x08],
        '8': [0x0E, 0x11, 0x0E, 0x11, 0x0E],
        '9': [0x0E, 0x11, 0x0F, 0x01, 0x0E],
    }
    
    char = str(letter).upper()
    if char not in font:
        # Return empty 5x7 bitmap for unknown characters
        return np.zeros((7, 5), dtype=np.uint8)
    
    char_data = font[char]
    bitmap = np.zeros((7, 5), dtype=np.uint8)
    
    for row in range(5):  # 5 rows of font data
        for col in range(5):  # 5 columns
            if char_data[row] & (1 << (4 - col)):
                bitmap[row + 1, col] = 1  # Offset by 1 row for better centering
    
    return bitmap

def create_text_bitmap(text, spacing=1):
    """Create a bitmap for text string"""
    letters = []
    max_height = 7  # 5x7 font + 1 pixel top/bottom padding
    
    # Create bitmaps for each letter
    for char in text:
        if char == ' ':
            # Add space (empty column)
            letters.append(np.zeros((max_height, 3), dtype=np.uint8))
        else:
            letters.append(create_letter_bitmap(char))
    
    # Calculate total width
    total_width = sum(letter.shape[1] for letter in letters) + (len(letters) - 1) * spacing
    
    # Create final bitmap
    result = np.zeros((max_height, total_width), dtype=np.uint8)
    
    # Composite letters
    x_offset = 0
    for letter in letters:
        height, width = letter.shape
        result[0:height, x_offset:x_offset + width] = letter
        x_offset += width + spacing
    
    return result

def bitmap_to_bytes(bitmap):
    """Convert numpy bitmap to byte array (row-major, 1 bit per pixel)"""
    height, width = bitmap.shape
    flat = bitmap.flatten()
    
    # Pack 8 pixels per byte (MSB first)
    bytes_array = bytearray()
    for i in range(0, len(flat), 8):
        byte = 0
        for bit in range(8):
            if i + bit < len(flat) and flat[i + bit] > 0:
                byte |= (0x80 >> bit)
        bytes_array.append(byte)
    
    return bytes(bytes_array)

def transform_to_printer_format(bitmap_bytes, width, height):
    """
    Convert bitmap to printer format:
    1. Row-major → Column-major (left-to-right, top-to-bottom)
    2. 16-bit word swap
    """
    BYTES_PER_COLUMN = height // 8  # 12 bytes for 96 pixels
    total_bytes = (width * height) // 8
    
    # Step 1: Convert to column-major (left-to-right, top-to-bottom)
    result = bytearray(total_bytes)
    
    for x in range(width):  # Left to right
        for y in range(height):  # Top to bottom
            # Read pixel from row-major bitmap
            row_idx = y * width + x
            byte_idx = row_idx // 8
            bit_pos = 7 - (row_idx % 8)  # MSB first
            pixel = (bitmap_bytes[byte_idx] >> bit_pos) & 1
            
            # Write to column-major (top-to-bottom within each column)
            # Column x stores pixels from top (y=0) to bottom (y=height-1)
            col_idx = x * BYTES_PER_COLUMN + y // 8
            bit_pos_out = y % 8
            
            if pixel:
                result[col_idx] |= (1 << bit_pos_out)
    
    # Step 2: 16-bit word swap (bottom-most 16 bit to top-most 16 bit, swap for the middle 4 segments as well)
    swapped = bytearray(total_bytes)
    
    for col in range(width):
        col_start = col * BYTES_PER_COLUMN
        src = col_start
        
        # Pattern: [0,1,2,3,4,5,6,7,8,9,10,11] → [10,11,8,9,6,7,4,5,2,3,0,1]
        
        swapped[col_start] = result[src + 10]
        swapped[col_start + 1] = result[src + 11]
        
        swapped[col_start + 2] = result[src + 8]
        swapped[col_start + 3] = result[src + 9]
        
        swapped[col_start + 4] = result[src + 6]
        swapped[col_start + 5] = result[src + 7]
        
        swapped[col_start + 6] = result[src + 4]
        swapped[col_start + 7] = result[src + 5]
        
        swapped[col_start + 8] = result[src + 2]
        swapped[col_start + 9] = result[src + 3]
        
        swapped[col_start + 10] = result[src]
        swapped[col_start + 11] = result[src + 1]
    
    return bytes(swapped)

def compress_and_generate_frames(bitmap, image_name, mtu_size=255):
    height, width = bitmap.shape
    print(f"\n{'='*70}")
    print(f"Generating frames for: {image_name} ({width}x{height})")
    print(f"{'='*70}")
    
    # Step 1: Convert bitmap to bytes
    bitmap_bytes = bitmap_to_bytes(bitmap)
    print(f"1. Bitmap → Bytes: {len(bitmap_bytes)} bytes")
    
    # Step 2: Transform to printer format (column-major, bottom-to-top)
    printer_format = transform_to_printer_format(bitmap_bytes, width, height)
    print(f"2. Transformed to printer format: {len(printer_format)} bytes")
    
    # Step 3: Calculate width split (in pixels/columns)
    BYTES_PER_COLUMN = height // 8  # 96 pixels = 12 bytes per column
    
    chunk_max_width = 85  # maximum width of each chunk
    chunk_remaining_width = width % chunk_max_width if width % 85 != 0 else chunk_max_width
    number_of_chunks = math.ceil(width / 85)
    
    print(f"3. Splitting {width} columns into {number_of_chunks} chunks:")
    print(f"   Base width per chunk: {chunk_max_width} columns ({chunk_max_width * BYTES_PER_COLUMN} bytes)")
    
    # Step 4: Split bitmap by width (columns), then compress each chunk
    frames = []
    column_offset = 0
    total_compressed = 0
    
    print(f"\n4. Processing chunks (splitting by width):")
    
    for chunk_idx in range(number_of_chunks):
        # Calculate width for this chunk (distribute remainder evenly)
        chunk_width = chunk_max_width
        
        # Calculate byte range for this chunk
        # In column-major format: each column is BYTES_PER_COLUMN bytes
        byte_offset = column_offset * BYTES_PER_COLUMN
        chunk_bytes = chunk_width * BYTES_PER_COLUMN
        
        # Extract THIS chunk (vertical slice of columns)
        chunk_data = printer_format[byte_offset:byte_offset + chunk_bytes]
        
        # Compress THIS chunk independently
        try:
            compressed_chunk = minilzo.compress(chunk_data)
        except Exception as e:
            print(f"   Chunk {chunk_idx + 1} compression failed: {e}")
            return None
        
        total_compressed += len(compressed_chunk)
        
        frame_width = chunk_remaining_width if chunk_idx + 1 == number_of_chunks else chunk_max_width
        frames_remaining = number_of_chunks - chunk_idx - 1
        
        # Create BLE frame
        new_frames = create_ble_frame(
            compressed_chunk,
            frames_remaining,
            frame_width,
            mtu_size
        )
        
        frames.extend(new_frames)
        
        print(f"   Chunk {chunk_idx + 1}/{number_of_chunks}: "
              f"width={chunk_width} cols, "
              f"uncompressed={chunk_bytes}B → compressed={len(compressed_chunk)}B "
              f"({len(compressed_chunk)*100/chunk_bytes:.1f}%), "
              f"frame_total={len(new_frames)}B, "
              f"remaining={frames_remaining}")
        
        column_offset += chunk_width
    
    print(f"\n5. Summary:")
    print(f"   Total chunks: {number_of_chunks}")
    print(f"   Total columns processed: {column_offset}/{width}")
    print(f"   Total uncompressed: {len(printer_format)}B")
    print(f"   Total compressed: {total_compressed}B ({total_compressed*100/len(printer_format):.1f}%)")
    
    return frames

def create_ble_frame(compressed_chunk, frames_remaining, chunk_width, mtu_size=255):
    frame = bytearray()
    
    # Header starts with Magic number 0x66

    frame.append(0x66)
    length = 17 + len(compressed_chunk) + 1
    frame.append(length & 0xFF)
    frame.append((length >> 8) & 0xFF)
    frame.extend(PRINTER_CONSTANT)
    frame.append(IMAGE_WIDTH & 0xFF)
    frame.append((IMAGE_WIDTH >> 8) & 0xFF)
    frame.append(chunk_width)
    frame.append((frames_remaining >> 8) & 0xFF)
    frame.append(frames_remaining & 0xFF)

    # End of Header
    frame.append(0x00)
    
    frame.extend(compressed_chunk)
    
    # Checksum (last byte)
    checksum = calculate_checksum(frame + b'\x00')
    frame.append(checksum)
    
    max_data_size = mtu_size - CID_0004_HEADER_SIZE
    if length > max_data_size:
        return [bytes(frame[:max_data_size]), bytes(frame[max_data_size:])]
    
    return [bytes(frame)]

def visualize_bitmap_matplotlib(bitmap, title="Bitmap Visualization", show_grid=True):
    """Visualize bitmap using matplotlib"""
    height, width = bitmap.shape
    
    fig, ax = plt.subplots(figsize=(width/10, height/10))
    
    # Display bitmap (invert colors for visualization: 0=white, 1=black)
    # But we want black pixels to show as black in visualization
    ax.imshow(bitmap, cmap='gray', vmin=0, vmax=1, aspect='auto')
    
    # Set title and labels
    ax.set_title(f"{title}\n{width}x{height} pixels")
    ax.set_xlabel("X (columns)")
    ax.set_ylabel("Y (rows)")
    
    # Add grid if requested
    if show_grid:
        ax.grid(True, color='red', linestyle='--', linewidth=0.5, alpha=0.3)
        ax.set_xticks(np.arange(-0.5, width, 10), minor=True)
        ax.set_yticks(np.arange(-0.5, height, 10), minor=True)
        ax.grid(True, which='minor', color='blue', linestyle=':', linewidth=0.2, alpha=0.2)
    
    # Set limits
    ax.set_xlim(-0.5, width - 0.5)
    ax.set_ylim(height - 0.5, -0.5)  # Invert y-axis to match image coordinates
    
    # Add pixel count info
    black_pixels = np.sum(bitmap == 1)
    white_pixels = np.sum(bitmap == 0)
    total_pixels = width * height
    
    info_text = f"Black: {black_pixels} ({black_pixels/total_pixels:.1%})\n"
    info_text += f"White: {white_pixels} ({white_pixels/total_pixels:.1%})"
    
    ax.text(0.02, 0.98, info_text, transform=ax.transAxes, 
            fontsize=9, verticalalignment='top',
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
    
    plt.tight_layout()
    plt.show()

def visualize_bitmap_ascii(bitmap, max_width=80, max_height=40):
    """Simple ASCII visualization of the bitmap"""
    height, width = bitmap.shape
    scale_x = width / max_width
    scale_y = height / max_height
    
    print("ASCII Visualization:")
    for y in range(0, min(height, max_height), max(1, int(scale_y))):
        line = ""
        for x in range(0, min(width, max_width), max(1, int(scale_x))):
            if bitmap[y, x] > 0:
                line += "█"  # Black pixel
            else:
                line += " "  # White pixel
        print(line)

def generate_python_frames_output(frames, test_name):
    """Generate Python code with frames"""
    print(f"\n{'='*70}")
    print(f"Python code for: {test_name}")
    print(f"{'='*70}")
    print("all_frames = [")
    
    for i, frame in enumerate(frames):
        hex_string = ' '.join(f"{b:02X}" for b in frame)
        if i < len(frames) - 1:
            print(f'    "{hex_string}",')
        else:
            print(f'    "{hex_string}"')
    
    print("]")

def generate_esp32_code(frames, test_name):
    """Generate ESP32 C++ code"""
    print(f"\n{'='*70}")
    print(f"ESP32 code for: {test_name}")
    print(f"{'='*70}")
    
    for i, frame in enumerate(frames):
        print(f"\n// Frame {i+1} - {len(frame)} bytes")
        print("printFrames.push_back({")
        
        for j in range(0, len(frame), 16):
            line = frame[j:j+16]
            hex_str = ', '.join(f"0x{b:02X}" for b in line)
            if j + 16 >= len(frame):
                print(f"    {hex_str}")
            else:
                print(f"    {hex_str},")
        
        print("});")

def verify_roundtrip(frames, expected_width):
    print(f"\n{'='*70}")
    print("Verifying roundtrip (decompress frames)")
    print(f"{'='*70}")
    
    decompressed_data = bytearray()
    BYTES_PER_COLUMN = IMAGE_HEIGHT // 8
    
    # Reassemble frames: if frame starts with 0x66 it's a new frame, otherwise append to previous
    assembled_frames = []
    current_frame = bytearray()
    
    for frame in frames:
        if frame[0] == 0x66:  # New frame starts with magic byte 0x66
            # Save previous frame if it exists
            if current_frame:
                assembled_frames.append(bytes(current_frame))
                current_frame.clear()
            # Start new frame
            current_frame.extend(frame)
        else:
            # Continuation frame - append to current frame
            current_frame.extend(frame)
    
    # Don't forget the last frame
    if current_frame:
        assembled_frames.append(bytes(current_frame))
    
    print(f"Original frames: {len(frames)}, Assembled frames: {len(assembled_frames)}")
    
    # Now process the assembled frames
    for i, frame in enumerate(assembled_frames):
        # Extract compressed chunk (skip header: magic(1) + length(2) + PRINTER_CONSTANT(8) + 
        # IMAGE_WIDTH(2) + chunk_width(1) + frames_remaining(2) + end_header(1) = 17 bytes)
        # and skip checksum (last byte)
        if len(frame) < 19:  # Minimum frame size
            print(f"   Frame {i+1}: Too small ({len(frame)} bytes)")
            continue
            
        compressed_chunk = frame[17:-1]  # Skip 17-byte header and last checksum byte
        
        try:
            dst_len = 9999 # weird API, max decompressed length has to be known beforehand or we fail.
            decompressed = minilzo.decompress(compressed_chunk, dst_len)
            decompressed_data.extend(decompressed)
            
            columns = len(decompressed) // BYTES_PER_COLUMN
            print(f"   Frame {i+1}: {len(compressed_chunk)}B → {len(decompressed)}B "
                  f"({columns} columns)")
        except Exception as e:
            print(f"   Frame {i+1}: Decompression failed - {e}")
            return False
    
    expected_size = (expected_width * IMAGE_HEIGHT) // 8
    
    if len(decompressed_data) == expected_size:
        print(f"\nSUCCESS! Decompressed {len(decompressed_data)} bytes "
              f"(expected {expected_size})")
        return True
    else:
        print(f"\nFAILED! Decompressed {len(decompressed_data)} bytes "
              f"(expected {expected_size})")
        return False

def plot_all_test_patterns(test_configs):
    """Plot all test patterns in a grid"""
    n_patterns = len(test_configs)
    cols = 3
    rows = (n_patterns + cols - 1) // cols
    
    fig, axes = plt.subplots(rows, cols, figsize=(15, 5*rows))
    axes = axes.flatten() if rows > 1 else [axes]
    
    for idx, config in enumerate(test_configs):
        if idx >= len(axes):
            break
            
        pattern = config[0]
        name = config[1]
        ax = axes[idx]
        
        # Create bitmap
        if pattern == "row_numbers" and len(config) > 2:
            spacing = config[2]
            bitmap = create_test_bitmap(IMAGE_WIDTH, IMAGE_HEIGHT, pattern, text_y=spacing)
        elif pattern in ["text", "letter"] and len(config) > 2:
            text_param = config[2]
            bitmap = create_test_bitmap(IMAGE_WIDTH, IMAGE_HEIGHT, pattern, 
                                        text=text_param, text_x=5, text_y=5)
        elif pattern == "y_varying_text":
            bitmap = create_test_bitmap(IMAGE_WIDTH, IMAGE_HEIGHT, pattern, text_x=8, text_y=9)
        else:
            bitmap = create_test_bitmap(IMAGE_WIDTH, IMAGE_HEIGHT, pattern)
        
        # Display bitmap
        ax.imshow(bitmap, cmap='gray', vmin=0, vmax=1, aspect='auto')
        ax.set_title(f"{name}\n({pattern})")
        ax.set_xlabel(f"{bitmap.shape[1]}px")
        ax.set_ylabel(f"{bitmap.shape[0]}px")
        ax.grid(True, color='red', linestyle='--', linewidth=0.3, alpha=0.3)
    
    # Hide unused subplots
    for idx in range(len(test_configs), len(axes)):
        axes[idx].axis('off')
    
    plt.suptitle(f"Test Bitmap Patterns ({IMAGE_WIDTH}x{IMAGE_HEIGHT})", fontsize=16, y=0.98)
    plt.tight_layout()
    plt.show()

# ============================================================================
# MAIN
# ============================================================================

if __name__ == "__main__":
    print("╔" + "="*68 + "╗")
    print("║" + "  MakeID L1 Printer - Frame Generator with Matplotlib".center(68) + "║")
    print("║" + "  Split FIRST, then compress each chunk".center(68) + "║")
    print("╚" + "="*68 + "╝")
    
    test_configs = [
        ("border", "Border Pattern"),
        ("all_white", "All White"),
        ("all_black", "All Black"), 
        ("diagonal", "Diagonal Lines"),
        ("stripes", "Vertical Stripes"),
        ("horizontal_stripes", "Horizontal Stripes"),
        ("checkerboard", "Checkerboard"),
        ("text", "Credits Song for My Death", "IN MY HEAD"),
        ("letter", "Letters ABC", "ABC123"),
        ("y_varying_text", "Y-Varying Numbers"),  # New: Numbers varying by row
        ("row_numbers", "Row Numbers", 15),  # New: Row numbers with spacing
    ]
    
    # Option 1: Plot all patterns in one figure
    print("\nGenerating visualization of all test patterns...")
    plot_all_test_patterns(test_configs)
    
    # Option 2: Process each pattern individually with detailed visualization
    print("\nProcessing each pattern individually:")
    
    for config in test_configs:
        pattern = config[0]
        name = config[1]
        
        print(f"\n{'='*70}")
        print(f"Processing: {name} ({pattern})")
        print(f"{'='*70}")
        
        # Create bitmap
        if pattern == "row_numbers" and len(config) > 2:
            spacing = config[2]
            bitmap = create_test_bitmap(IMAGE_WIDTH, IMAGE_HEIGHT, pattern, text_y=spacing)
        elif pattern in ["text", "letter"] and len(config) > 2:
            text_param = config[2]
            bitmap = create_test_bitmap(IMAGE_WIDTH, IMAGE_HEIGHT, pattern, 
                                        text=text_param, text_x=5, text_y=5)
        elif pattern == "y_varying_text":
            bitmap = create_test_bitmap(IMAGE_WIDTH, IMAGE_HEIGHT, pattern, text_x=8, text_y=9)
        else:
            bitmap = create_test_bitmap(IMAGE_WIDTH, IMAGE_HEIGHT, pattern)
        
        # Show detailed matplotlib visualization
        # visualize_bitmap_matplotlib(bitmap, title=name)
        
        # Show ASCII visualization
        visualize_bitmap_ascii(bitmap)
        
        # Generate frames
        frames = compress_and_generate_frames(bitmap, name)
    
        if frames:
            # Verify roundtrip
            if verify_roundtrip(frames, IMAGE_WIDTH):
                # Generate code (optional - uncomment if needed)
                generate_python_frames_output(frames, name)
                generate_esp32_code(frames, name)
                pass

            print(f"\n{'='*70}\n")
        else:
            print(f"Failed to generate frames for {name}\n")

    print("╔" + "="*68 + "╗")
    print("║" + "  Generation Complete!".center(68) + "║")
    print("╚" + "="*68 + "╝")
