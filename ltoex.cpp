﻿#include "AES.h"
#include "SLDC.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _MSC_VER
	#include "unistd-win.h"
#else
	#include <unistd.h>
#endif
#include <memory>
#include <fstream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <string.h>


int main(int argc, char** argv)
{
	bool deviceInput = false;
	// MSVC support is for debugging only
#ifdef _MSC_VER
	auto fp = fopen("sinkusrenc.bin", "rb");
	int fd = fileno(fp);
#else
	int fd = STDIN_FILENO;
	if (argc == 3)
	{
		deviceInput = true;
		auto fp = fopen(argv[2], "rb");
		fd = fileno(fp);
	}
	else if (argc != 2)
	{
		fprintf(stderr, "Usage: ltoex keyfile.key [/dev/nst0]\n");
		return 1;
	}
#endif

	// Load given argument as stenc-compatible key file
	std::array<uint8_t, 32> key;
	{
		std::fstream keyStream;
		keyStream.open(argv[1]);
		std::string hexKey((std::istreambuf_iterator<char>(keyStream)), (std::istreambuf_iterator<char>()));
		for (size_t i = 0; i < key.size(); ++i)
		{
			std::string byte = hexKey.substr(i * 2, 2);
			key[i] = static_cast<uint8_t>(strtol(byte.c_str(), nullptr, 16));
		}
	}


	// Encrypted blocks are not a consistent length on LTO, so we must prepare for the worst
	// I feel bad doing it, but let's be honest who's going to bat an eyelid at 32mb ram usage in 2020
	constexpr size_t inputBufferSize = 16 * 1024 * 1024;
	constexpr size_t inputBlockSize = 4 * 1024 * 1024;
	constexpr size_t decryptedBufferSize = 8 * 1024 * 1024;
	assert(inputBufferSize % inputBlockSize == 0);
	std::unique_ptr<uint8_t[]> inputBuffer = std::make_unique<uint8_t[]>(inputBufferSize);
	size_t inputBufferPtr = 0;
	std::unique_ptr<uint8_t[]> decryptedBuffer = std::make_unique<uint8_t[]>(decryptedBufferSize);
	std::vector<uint8_t> outputBuffer;
	outputBuffer.reserve(decryptedBufferSize);

	// If we're reading directly from the tape drive, we don't need to do the questionable searching for headers to determine block sizes
	if (deviceInput)
	{
		while (true)
		{
			ssize_t readBytes = read(fd, inputBuffer.get(), inputBlockSize);
			//fprintf(stderr, "ReadBytes: %i\n", readBytes);

			if (readBytes > 0)
			{
				if (!AES::Decrypt(key.data(), inputBuffer.get(), readBytes, decryptedBuffer.get(), decryptedBufferSize))
				{
					fprintf(stderr, "Failed AES decryption (wrong key?)\n");
					return 1;
				}

			#if 1
				SLDC sldc;

				if (!sldc.Extract(decryptedBuffer.get(), readBytes - AES::EXTRA_BYTES, outputBuffer))
				{
					fprintf(stderr, "Failed SLDC decompression (probably a bug, corruption should be caught at AES stage)\n");
					return 1;
				}

			#if !defined(_MSC_VER) 
				write(STDOUT_FILENO, outputBuffer.data(), (int)outputBuffer.size());
			#endif
				outputBuffer.clear();
			#else
				write(STDOUT_FILENO, decryptedBuffer.get(), readBytes - AES::EXTRA_BYTES);
			#endif
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		// Use the AAD and 4 null bytes as a header we can search for to find block boundaries
		// In an ideal world this would also use the IV, but 20 bytes is already decent odds against collision
		constexpr uint32_t headerSize = 32;
		ssize_t readBytes = read(fd, inputBuffer.get(), inputBlockSize);
		//fprintf(stderr, "ReadBytes: %i\n", readBytes);
		if (readBytes < headerSize)
		{
			fprintf(stderr, "Failed to read 32 byte block header (readBytes: %i, err: %s)\n", readBytes, strerror(errno));
			return 1;
		}
		inputBufferPtr += readBytes;
		for (size_t i = 0; i < 4; ++i)
		{
			if (inputBuffer.get()[(headerSize - 1) - i] != 0)
			{
				fprintf(stderr, "Unexpected bytes in header\n");
				return 1;
			}
		}
		std::array<uint8_t, 16> aad;
		std::copy(inputBuffer.get(), inputBuffer.get() + 16, aad.begin());

		do
		{
			// Read as much as we can
			while (inputBufferPtr < inputBufferSize - inputBlockSize)
			{
				readBytes = read(fd, inputBuffer.get() + inputBufferPtr, inputBlockSize);
				//fprintf(stderr, "ReadBytes: %i\n", readBytes);
				if (readBytes <= 0)
				{
					break;
				}
				inputBufferPtr += readBytes;
			}

			// Find and process as many full records as we can find, including the trailing record if we did not fill the input buffer
			uint8_t* pInputEnd = inputBuffer.get() + inputBufferPtr;
			uint8_t* pLastProcessedRecordEnd = pInputEnd;

			uint8_t* pRecordStart = std::search(
				inputBuffer.get(),
				pInputEnd,
				aad.begin(),
				aad.end());
			uint8_t* pRecordEnd = std::search(
				pRecordStart + headerSize,
				pInputEnd,
				aad.begin(),
				aad.end());

			while (pRecordStart != pInputEnd)
			{
				if (pRecordStart != pInputEnd && (pRecordEnd != pInputEnd || readBytes == 0))
				{
					for (size_t i = 0; i < 4; ++i)
					{
						if (*(pRecordStart + (headerSize - 1) - i) != 0)
						{
							fprintf(stderr, "Unexpected bytes in record header\n");
							return 1;
						}
					}

					pLastProcessedRecordEnd = pRecordEnd;
					size_t recordLength = pRecordEnd - pRecordStart;
					//fprintf(stderr, "Record start: %i, record length: %i, inputBufferPtr: %i\n", pRecordStart - inputBuffer.get(), recordLength, inputBufferPtr);
					if (!AES::Decrypt(key.data(), pRecordStart, recordLength, decryptedBuffer.get(), decryptedBufferSize))
					{
						fprintf(stderr, "Failed AES decryption (wrong key?)\n");
						return 1;
					}

				#if 1
					SLDC sldc;

					if (!sldc.Extract(decryptedBuffer.get(), recordLength - AES::EXTRA_BYTES, outputBuffer))
					{
						fprintf(stderr, "Failed SLDC decompression (probably a bug, corruption should be caught at AES stage)\n");
						return 1;
					}

				#if !defined(_MSC_VER) 
					write(STDOUT_FILENO, outputBuffer.data(), (int)outputBuffer.size());
				#endif
					outputBuffer.clear();
				#else
					write(STDOUT_FILENO, decryptedBuffer.get(), recordLength - AES::EXTRA_BYTES);
				#endif
				}

				pRecordStart = pRecordEnd;

				if (pRecordStart < pInputEnd)
				{
					pRecordEnd = std::search(
						pRecordStart + headerSize,
						pInputEnd,
						aad.begin(),
						aad.end());
				}
			}

			// Copy overflow back to the start of the buffer
			//fprintf(stderr, "Overflow: %i\n", pInputEnd - pLastProcessedRecordEnd);
			std::copy(pLastProcessedRecordEnd, pInputEnd, inputBuffer.get());
			inputBufferPtr = pInputEnd - pLastProcessedRecordEnd;
		} while (inputBufferPtr > 0);
	}

	return 0;
}
