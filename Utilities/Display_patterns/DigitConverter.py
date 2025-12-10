# rearrange_bits.py

INPUT_FILE = "14-Segment-ASCII_BIN_GitHub.txt"
OUTPUT_FILE = "converted_patterns.txt"

# desired new ordering of bit positions
ORDER = [11, 10, 8, 1, 2, 3, 0, 7, 4, 14, 5, 13, 6, 9, 12]

def rearrange_bits(bits: str) -> str:
    """Return a new 15-bit string rearranged according to ORDER."""
    return ''.join(bits[i] for i in ORDER)

def process_line(line: str) -> str:
    """Process a single line, rearranging only the binary literal."""
    if ',' not in line:
        return line  # leave lines without comma unchanged

    binary_part, rest = line.split(',', 1)
    binary_part = binary_part.strip()

    # sanity check: must be exactly 15 bits
    if len(binary_part) == 15 and all(c in "01" for c in binary_part):
        new_binary = rearrange_bits(binary_part)
        return f"{new_binary},{rest}"

    return line  # if it doesn't match expectations, leave unchanged

def main():
    with open(INPUT_FILE, "r") as inp, open(OUTPUT_FILE, "w") as out:
        for line in inp:
            out.write(process_line(line))

if __name__ == "__main__":
    main()
