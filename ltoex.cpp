#include "AES.h"
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


int main(int argc, char** argv)
{
	// Load given argument as stenc-compatible key
	std::array<uint8_t, 32> key;
	{
		std::fstream keyStream;
		keyStream.open(argv[1]);
		std::string hexKey((std::istreambuf_iterator<char>(keyStream)), (std::istreambuf_iterator<char>()));
		for (int i = 0; i < key.size(); ++i)
		{
			std::string byte = hexKey.substr(i * 2, 2);
			key[i] = static_cast<uint8_t>(strtol(byte.c_str(), nullptr, 16));
		}
	}


#ifdef _MSC_VER
	auto fp = fopen("ascii1.bin", "rb");
	int fd = fileno(fp);
#else
	int fd = STDIN_FILENO;
#endif

	struct stat st;
	fstat(fd, &st);
	size_t inputBufferSize = strtol(argv[2], nullptr, 10);
	std::unique_ptr<uint8_t[]> inputBuffer = std::make_unique<uint8_t[]>(inputBufferSize);
	std::unique_ptr<uint8_t[]> decryptedBuffer = std::make_unique<uint8_t[]>(inputBufferSize);
	std::vector<uint8_t> outputBuffer;
	outputBuffer.reserve(inputBufferSize);

	// This read loop is comically bad
	ssize_t readBytes = 0;
	do
	{
		size_t blockBytes = 0;
		readBytes = 0;
		do
		{
			readBytes = read(fd, inputBuffer.get() + blockBytes, int(inputBufferSize - blockBytes));
			if (readBytes > 0)
			{
				blockBytes += readBytes;
			}
		}
		while (blockBytes < inputBufferSize && readBytes > 0);

		if (blockBytes > 0)
		{
			printf("blockbytes: %i\n", blockBytes);
			printf("inputBufferSize: %i\n", inputBufferSize);
			//assert(blockBytes >= decryptedBufferSize);
			if (!AES::Decrypt(key.data(), inputBuffer.get(), blockBytes, decryptedBuffer.get(), inputBufferSize))
			{
				printf("Failed AES decryption (wrong key?)\n");
				return 1;
			}

		#if 1
			SLDC sldc;
			sldc.Extract(decryptedBuffer.get(), blockBytes - AES::EXTRA_BYTES, outputBuffer);

			write(STDOUT_FILENO, outputBuffer.data(), (int)outputBuffer.size());
			outputBuffer.clear();
		#else
			write(STDOUT_FILENO, decryptedBuffer.get(), blockBytes - AES::EXTRA_BYTES);
		#endif
		}
	} 
	while (readBytes > 0);

	return 0;
}
