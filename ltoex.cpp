#include "ltoex.h"
#include "AES.h"
#include "SLDC.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory>
#include <fstream>
#include <cassert>
#include <vector>


int main(int argc, char** argv)
{
	//chdir("/mnt/c/github/ltoex/out/build/WSL-Debug");
	// Load given argument as stenc-compatible key
	std::array<uint8_t, 32> key;
	{
		std::fstream keyStream;
	#ifdef _MSC_VER
		keyStream.open("tape.key");
	#else
		keyStream.open(argv[1]);
	#endif
		std::string hexKey((std::istreambuf_iterator<char>(keyStream)), (std::istreambuf_iterator<char>()));
		for (int i = 0; i < key.size(); ++i)
		{
			std::string byte = hexKey.substr(i * 2, 2);
			key[i] = static_cast<uint8_t>(strtol(byte.c_str(), nullptr, 16));
		}
	}

	//int fd = STDIN_FILENO;
	auto fp = fopen("owo1.bin", "rb");
	int fd = fileno(fp);

	struct stat st;
	fstat(fd, &st);
#ifdef _MSC_VER
	size_t inputBufferSize = 4096;
#else
	size_t inputBufferSize = st.st_blksize;
#endif
	size_t decryptedBufferSize = inputBufferSize - AES::EXTRA_BYTES;
	//size_t outputBufferSize = decryptedBufferSize;
	std::unique_ptr<uint8_t[]> inputBuffer = std::make_unique<uint8_t[]>(inputBufferSize);
	std::unique_ptr<uint8_t[]> decryptedBuffer = std::make_unique<uint8_t[]>(decryptedBufferSize);
	//std::unique_ptr<uint8_t[]> outputBuffer = std::make_unique<uint8_t[]>(outputBufferSize);
	std::vector<uint8_t> outputBuffer;
	outputBuffer.reserve(decryptedBufferSize);

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

		assert(blockBytes <= decryptedBufferSize);
		AES::Decrypt(key.data(), inputBuffer.get(), blockBytes, decryptedBuffer.get(), decryptedBufferSize);

		SLDC sldc(decryptedBuffer.get(), blockBytes - AES::EXTRA_BYTES);
		sldc.Extract(outputBuffer);

		write(STDOUT_FILENO, outputBuffer.data(), (int)outputBuffer.size());
		//write(STDOUT_FILENO, decryptedBuffer.get(), blockBytes - AES::EXTRA_BYTES);
	} 
	while (readBytes > 0);

	return 0;
}
