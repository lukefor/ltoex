# ltoex
**C++ implementation of LTO Ultrium tape AES-GCM decryption and SLDC (Streaming Lossless Data Compression) decompression**

## Description

This program is the result of a rabbit hole started when I found [IBM LTO-4 drives cannot read encrypted tapes from HP LTO-4 drives](https://darkimmortal.com/lto4-encryption-woes/) (vice-versa is okay). It is intended mainly as a solution to this problem - the IBM drive can read the raw encrypted data from the tape, which `ltoex` decrypts and decompresses in software.

Another possible use is data recovery - it may be possible to partially recover corrupted blocks with this.

The decryption is handled by OpenSSL (AES-GCM with 256 bit key, 16 byte AAD, 16 byte tag and 96 bit IV), and the SLDC decompression is implemented from scratch following *ECMA-321* and *ISO/IEC 22091:2002*.

A further description of the secrets uncovered in the LTO format during development can be found here: https://darkimmortal.com/the-secrets-of-lto-tape/

## Building

```
cmake -DCMAKE_BUILD_TYPE=Release .
make
```
(move `ltoex` somewhere nice like `/usr/local/bin`)

## Usage

#### Set up raw reads with stenc:
Before anything else, you must put your drive into raw read mode, such as with `stenc`. HP drives will balk at this if the tape was not written with --unprotect set.
```
stenc -f /dev/nst0 -e rawread -k /path/to/my/stenc.key -a 1 --unprotect
```

#### Tar listing example:
These examples will print the contents of a tar tape. 

The first argument to `ltoex` is the path to a key file created by `stenc`, which is a 256-bit key in ascii hex digits. The second is an *optional* block device to read from - this must be a physical tape drive.

```
ltoex /path/to/my/stenc.key /dev/nst0 | tar -tvf -
```

You can also pipe `cat`/`dd`/`mbuffer` etc, in which case leave out the second argument to `ltoex`. When piping, use a block size of 2x the tape block size, and do not specify `iflag=fullblock` to `dd`. If your tape block size is larger than 2MB (2MB itself is fine and recommended), you must use the pipe interface. The pipe interface reconstructs variable block sizes heuristically, so it is potentially imperfect compared to directly reading from the drive.

## Notes

`ltoex` currently achieves ~150MB/s (read) on an Ivy Bridge (v2) Xeon CPU against mostly uncompressible (Scheme 2) data. Against 'mixed' data (a tar archive of /usr on a Debian system), it achieves approximately 70MB/s (read) on the same hardware.

`ltoex` would need to be changed, probably quite significantly, for LTO-5 and above. It is only suitable for LTO-4 in its current form. However, the bug is probably fixed in IBM LTO-5 drives, so it won't be much use anyway.

Only Linux is supported, but it will probably work on other platforms with similar tape drivers. MSVC is supported for debug purposes only, as Visual Studio is my dev environment of choice.

Currently the only supported use case is reading tapes on an IBM LTO-4 drive that were written on an HP LTO-4 drive. Getting an HP drive to return its own raw/encrypted data is easier said than done, and IBM SLDC data does not appear to follow spec (although HP drives are able to understand it, somehow).
