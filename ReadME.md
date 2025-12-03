<h1><img width="500" alt="FatRec Banner" src="/doc/fat-rec-32-banner.png"></h1>

[![Tests](https://github.com/kazisean/FatRec32/actions/workflows/tests.yml/badge.svg)](https://github.com/kazisean/FatRec32/actions/workflows/tests.yml) [![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.cppreference.com/w/c) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# FatRec32
FatRec32 is a high-performance file recovery tool designed to retrieve deleted files from FAT32 file systems.

## Features
**Versatile Recovery Options** :
  - Recover specific files
  - Bulk recovery of all deleted files
  - SHA1 validation for recovered files

## Installation

### Prerequisites
- **Linux:** `sudo apt-get install build-essential libssl-dev`
- **macOS:** `xcode-select --install` (OpenSSL is typically included)

### Build
```bash
git clone https://github.com/yourusername/fatrec32.git
cd fatrec32
make
```

## Usage

```
Usage: fatrec32 disk <options>
  -i                     Print the file system information.
  -l                     List the root directory.
  -r filename [-s sha1]  Recover a contiguous file.
  -R filename -s sha1    Recover a possibly non-contiguous file.
  -ra filename           Recover all files with the given name.
  -all                   Recover all deleted files.
```

### Examples

```bash
# Print information about the file system
./fatrec32 sample.disk -i

# List files in the root directory
./fatrec32 sample.disk -l

# Recover a specific deleted file
./fatrec32 sample.disk -r document.pdf

# Recover a file and verify its integrity with SHA1
./fatrec32 sample.disk -r document.pdf -s 5baa61e4c9b93f3f0682250b6cf8331b7ee68fd8

# Recover all deleted files
./fatrec32 sample.disk -all

# Recover all instances of a specific filename (if there were multiple)
./fatrec32 sample.disk -ra file.txt
```

## Technical Details

FatRec32 works by scanning the FAT32 filesystem at the byte level, identifying deleted file entries and reconstructing file contents based on cluster chain analysis. 

- **Memory-Mapped I/O**: Directly maps disk sectors to memory for faster access
- **Cluster Chain Recovery**: Advanced algorithms to reconstruct fragmented files
- **File Carving Techniques**: Signature-based recovery for specific file types
- **Cryptographic Validation**: SHA1 hashing to verify file post-recovery

## Contributing

Contributions to FatRec32 are welcome! Whether it's bug reports, feature requests, or code contributions, please feel open an issue.

## License

This project is licensed under the MIT License.
