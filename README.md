# ltoex
**C++ implementation of LTO Ultrium tape AES-GCM decryption and SLDC (Streaming Lossless Data Compression) decompression**

## Description

This program is the result of a rabbit hole started when I found [IBM LTO-4 drives cannot read encrypted tapes from HP LTO-4 drives](https://darkimmortal.com/lto4-encryption-woes/) (vice-versa is okay). It is intended mainly as a solution to this problem - the IBM drive can read the raw encrypted data from the tape, which ltoex decrypts and decompresses in software.

Another possible use is data recovery - it may be possible to partially recover corrupted blocks with this.

The decryption is handled by OpenSSL (AES-GCM with 256 bit key, 16 byte AAD, 16 byte tag and 96 bit IV), and the SLDC decompression is implemented from scratch following *ECMA-321* and *ISO/IEC 22091:2002*.

A further description of the secrets uncovered in the LTO format during development can be found here: https://darkimmortal.com/the-secrets-of-lto-tape/

## Building

```
cmake .
make
(move ltoex somewhere nice like /usr/local/bin)
```

## Usage

```
stenc -f /dev/nst0 -e rawread -k /path/to/my/stenc.key -a 1 --unprotect
dd if=/dev/nst0 bs=16M | ltoex /path/to/my/stenc.key | tar -tvf -
```

This example usage will print the contents of a tar tape. The usage of `dd` with a 16MB block size is important - you cannot swap this out for `cat` or similar. `mbuffer` can be used but it must be with a 16MB block size. **The 16MB block size has no relation to the actual block size of the tape.**

The only argument to `ltoex` is the path to a key file created by `stenc`, which is a 256-bit key in ascii hex digits.

## Notes

This currently achieves ~45MB/s on a Sandy Bridge equivalent Xeon CPU. Bottleneck by far is the SLDC decompression. Work is planned to improve this, but it is currently fast enough for tape streaming.

This would need to be changed, probably quite significantly, for LTO-5 and above. It is only suitable for LTO-4 in its current form. However, the bug is probably fixed in IBM LTO-5 drives, so it won't be much use anyway.

Only Linux is supported, but it will probably work on other platforms with similar tape drivers. MSVC is supported for debug purposes only, as Visual Studio is my dev environment of choice.
