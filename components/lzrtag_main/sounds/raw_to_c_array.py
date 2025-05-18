import sys
import os
import subprocess
import tempfile

def convert_wav_to_c_array(input_wav_file, output_h_file_path, target_samplerate=16000):
    base_name = os.path.splitext(os.path.basename(output_h_file_path))[0]
    array_name = "sound_data_" + base_name.replace("-", "_").replace(" ", "_")
    header_guard = base_name.upper().replace("-", "_").replace(" ", "_") + "_H_"

    # Create a temporary file for the raw PCM data
    with tempfile.NamedTemporaryFile(suffix=".raw", delete=False) as tmp_raw_file_obj:
        tmp_raw_file_name = tmp_raw_file_obj.name
    
    try:
        # Step 1: Convert WAV to raw 8-bit unsigned PCM using ffmpeg
        ffmpeg_command = [
            "ffmpeg",
            "-i", input_wav_file,
            "-f", "u8",              # Output format: 8-bit unsigned
            "-ac", "1",             # Output channels: 1 (mono)
            "-ar", str(target_samplerate), # Output sample rate
            tmp_raw_file_name,
            "-y"                    # Overwrite output file if it exists
        ]
        print(f"Running ffmpeg: {' '.join(ffmpeg_command)}")
        result = subprocess.run(ffmpeg_command, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error running ffmpeg for {input_wav_file}:")
            print(f"STDOUT: {result.stdout}")
            print(f"STDERR: {result.stderr}")
            return # Exit if ffmpeg fails
        print(f"ffmpeg conversion successful for {input_wav_file} to {tmp_raw_file_name}")

        # Step 2: Convert raw PCM to C array
        with open(tmp_raw_file_name, 'rb') as f_in, open(output_h_file_path, 'w') as f_out:
            f_out.write(f"#ifndef {header_guard}\n")
            f_out.write(f"#define {header_guard}\n\n")
            f_out.write("#include <stdint.h>\n\n")
            f_out.write(f"// Source: {os.path.basename(input_wav_file)}\n")
            f_out.write(f"// Converted to {target_samplerate} Hz, 8-bit unsigned mono PCM\n")
            f_out.write(f"static const uint8_t {array_name}[] = {{\n    ")
            
            byte_count = 0
            bytes_written_in_line = 0
            byte = f_in.read(1)
            while byte:
                f_out.write(f"0x{byte.hex()}, ")
                byte_count += 1
                bytes_written_in_line +=1
                if bytes_written_in_line % 12 == 0:
                    f_out.write("\n    ")
                    bytes_written_in_line = 0
                byte = f_in.read(1)
            
            # Remove trailing comma and space if bytes were written
            if byte_count > 0:
                # Seek back 2 characters (", ") from the current position and truncate
                f_out.seek(f_out.tell() - 2)
                f_out.truncate()

            f_out.write("\n};\n\n")
            f_out.write(f"// Size of {array_name} in bytes\n")
            f_out.write(f"#define {array_name.upper()}_LEN {byte_count}\n\n")
            f_out.write(f"#endif /* {header_guard} */\n")
        print(f"Generated {output_h_file_path} with array {array_name} ({byte_count} bytes)")

    except IOError as e:
        print(f"Error processing file: {e}")
    except FileNotFoundError:
        print("Error: ffmpeg not found. Please ensure ffmpeg is installed and in your system's PATH.")
    finally:
        # Clean up the temporary raw file
        if os.path.exists(tmp_raw_file_name):
            os.remove(tmp_raw_file_name)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python raw_to_c_array.py <input_root_directory> <output_root_directory>")
        print("Example: python raw_to_c_array.py sounds/wav_sources/ components/lzrtag_main/sounds/lzrtag-sfx/")
        sys.exit(1)
    
    input_root_dir = sys.argv[1]
    output_root_dir = sys.argv[2]

    if not os.path.isdir(input_root_dir):
        print(f"Error: Input directory '{input_root_dir}' not found or is not a directory.")
        sys.exit(1)

    # Ensure output root directory exists, or create it
    if not os.path.exists(output_root_dir):
        try:
            os.makedirs(output_root_dir)
            print(f"Created output root directory: {output_root_dir}")
        except OSError as e:
            print(f"Error: Could not create output root directory {output_root_dir}: {e}")
            sys.exit(1)
    elif not os.path.isdir(output_root_dir):
        print(f"Error: Output path '{output_root_dir}' exists but is not a directory.")
        sys.exit(1)

    processed_files_count = 0
    print(f"Starting batch conversion from '{input_root_dir}' to '{output_root_dir}'...")
    for root, _, files in os.walk(input_root_dir):
        for file in files:
            if file.lower().endswith(".wav"):
                input_wav_file_path = os.path.join(root, file)
                
                # Determine the relative path of the wav file with respect to the input_root_dir
                relative_wav_path = os.path.relpath(input_wav_file_path, input_root_dir)
                
                # Construct the output .h file path
                # The output filename will be <original_basename>_raw.h
                # The subdirectory structure under output_root_dir will mirror that under input_root_dir
                wav_basename, _ = os.path.splitext(os.path.basename(relative_wav_path))
                output_h_filename = wav_basename + "_raw.h"
                
                relative_subdir = os.path.dirname(relative_wav_path)
                output_h_file_target_dir = os.path.join(output_root_dir, relative_subdir)
                output_h_file_full_path = os.path.join(output_h_file_target_dir, output_h_filename)
                
                # Ensure the specific output directory for this .h file exists
                if not os.path.exists(output_h_file_target_dir):
                    try:
                        os.makedirs(output_h_file_target_dir)
                        print(f"Created directory: {output_h_file_target_dir}")
                    except OSError as e:
                        print(f"Error: Could not create directory {output_h_file_target_dir}: {e}")
                        continue # Skip this file if its directory cannot be created
                
                print(f"Processing: {input_wav_file_path}  ==>  {output_h_file_full_path}")
                try:
                    convert_wav_to_c_array(input_wav_file_path, output_h_file_full_path)
                    processed_files_count += 1
                except Exception as e:
                    print(f"An error occurred while processing {input_wav_file_path}: {e}")

    if processed_files_count == 0:
        print(f"No .wav files were found and processed in '{input_root_dir}'.")
    else:
        print(f"Successfully processed {processed_files_count} .wav file(s).")
