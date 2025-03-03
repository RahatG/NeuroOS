#!/usr/bin/env python3

import sys
import struct

def check_multiboot(filename):
    with open(filename, 'rb') as f:
        # Read the first 8KB of the file
        data = f.read(8192)
        
        # Search for the multiboot2 magic number (0xE85250D6)
        for i in range(0, len(data) - 4, 4):
            magic = struct.unpack('<I', data[i:i+4])[0]
            if magic == 0xE85250D6:
                print(f"Found multiboot2 header at offset {i}")
                
                # Read the architecture field
                arch = struct.unpack('<I', data[i+4:i+8])[0]
                print(f"Architecture: {arch}")
                
                # Read the header length field
                header_length = struct.unpack('<I', data[i+8:i+12])[0]
                print(f"Header length: {header_length}")
                
                # Read the checksum field
                checksum = struct.unpack('<I', data[i+12:i+16])[0]
                print(f"Checksum: 0x{checksum:08x}")
                
                # Verify the checksum
                calculated_checksum = 0xFFFFFFFF - (0xE85250D6 + arch + header_length) + 1
                print(f"Calculated checksum: 0x{calculated_checksum:08x}")
                
                if checksum == calculated_checksum:
                    print("Checksum is valid")
                else:
                    print("Checksum is invalid")
                
                return True
        
        print("No multiboot2 header found")
        return False

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <filename>")
        sys.exit(1)
    
    check_multiboot(sys.argv[1])
