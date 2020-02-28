#include "ltoex.h"
#include "AES.h"
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

	struct stat st;
	fstat(STDIN_FILENO, &st);
	size_t inputBlockSize = st.st_blksize;
	size_t decryptedBlockSize = inputBlockSize - AES::EXTRA_BYTES;
	std::unique_ptr<uint8_t[]> inputBuffer = std::make_unique<uint8_t[]>(inputBlockSize);
	std::unique_ptr<uint8_t[]> decryptedBuffer = std::make_unique<uint8_t[]>(decryptedBlockSize);
	//printf("inputBlockSize: %i\n", inputBlockSize);

	ssize_t blockBytes = 0;
	ssize_t in = 0;
	do
	{
		blockBytes = 0;
		in = 0;
		do
		{
			in = read(STDIN_FILENO, inputBuffer.get() + blockBytes, inputBlockSize - blockBytes);
			if (in > 0)
			{
				blockBytes += in;
			}
			//printf("in: %i\n", in);
		}
		while (blockBytes < inputBlockSize && in > 0);

		assert(blockBytes <= decryptedBlockSize);
		AES::Decrypt(key.data(), inputBuffer.get(), blockBytes, decryptedBuffer.get(), decryptedBlockSize);

		write(STDOUT_FILENO, decryptedBuffer.get(), blockBytes - AES::EXTRA_BYTES);
	} 
	while (in > 0);

	return 0;
}
