# Secure TAR Archive Extractor

Secure TAR Archive Extractor is designed to extract TAR archives in a way that preserves all extended attributes (xattrs), 
including `security.ima` and `security.evm`, ensuring compatibility with Linux IMA/EVM requirements.

## Motivation

Traditional archive extraction methods (such as those utilized by libarchive's default `archive_write_disk` mechanism) 
may modify files after security attributes (like security.evm) have been set. 
This can violate IMA/EVM constraints and result in appraisal failures under enforcement. 
The Secure TAR Archive Extractor avoids these issues by ensuring the file metadata and content exactly match the integrity and security signatures.

## Implementation Overview

1. Uses `mknod()` to create files without opening them immediately.
2. Applies all extended attributes (including IMA/EVM-related attributes) before any file I/O operations.
3. Writes file content after the xattrs are securely in place.
4. Restores timestamps, ownership, and permissions to match the originally signed metadata.

## Dependencies

- libarchive 
- Linux Kernel with support for extended attributes and IMA/EVM (especially critical if in enforced mode).

## Build

 ```
 cmake .
 make
 ```

## Usage

To extract an archive using this secure mechanism, run:

 ```
./xattrs-extract-archive archive.tar.gz /target/path
 
 ```