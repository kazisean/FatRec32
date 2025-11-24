<h1><img width="500" alt="FatRec Banner" src="/doc/fat-rec-32-banner.png"></h1>

[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.cppreference.com/w/c) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# 💾 FatRec32
FatRec32 is a high-performance file recovery tool designed to retrieve deleted files from FAT32 file systems.

## Features
**Versatile Recovery Options** :
  - Recover specific files with name matching
  - Bulk recovery of all deleted files
  - Special handling for non-contiguous files
  - SHA1 validation for recovered files

## 📥 Installation

### Prerequisites

- GCC or compatible C compiler
- OpenSSL development libraries
- Make

### Build from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/fatrec32.git
cd fatrec32

# Build using make
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
./fatrec32 fat32.disk -i

# List files in the root directory
./fatrec32 fat32.disk -l

# Recover a specific deleted file
./fatrec32 fat32.disk -r document.pdf

# Recover a file and verify its integrity with SHA1
./fatrec32 fat32.disk -r document.pdf -s 5baa61e4c9b93f3f0682250b6cf8331b7ee68fd8

# Recover all deleted files
./fatrec32 fat32.disk -all

# Recover all instances of a specific filename
./fatrec32 fat32.disk -ra backup.zip
```

## Technical Details

FatRec32 works by scanning the FAT32 filesystem at the byte level, identifying deleted file entries and reconstructing file contents based on cluster chain analysis. 

- **Memory-Mapped I/O**: Directly maps disk sectors to memory for faster access
- **Cluster Chain Recovery**: Advanced algorithms to reconstruct fragmented files
- **File Carving Techniques**: Signature-based recovery for specific file types
- **Cryptographic Validation**: SHA1 hashing to verify file post-recovery

## Contributing

Contributions to FatRec32 are welcome! Whether it's bug reports, feature requests, or code contributions, please feel open an issue and submit a pull request.

1. Raise an issue + Fork the repository ⑂
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Added this amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request 🎉

## License

This project is licensed under the MIT License.
