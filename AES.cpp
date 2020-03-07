#include "AES.h"
#include <openssl/evp.h>
#include <cassert>


namespace AES
{
    bool Decrypt(const uint8_t* pKey, const uint8_t* pEncrypted, size_t encryptedLength, uint8_t* pOutput, size_t outputLength)
    {
        assert(encryptedLength <= outputLength + EXTRA_BYTES);
        int outlen = 0;
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);

        // Key + IV (16:28)
        EVP_DecryptInit_ex(ctx, NULL, NULL, pKey, pEncrypted + 16);               
        // AAD (0:16)
        EVP_DecryptUpdate(ctx, NULL, &outlen, pEncrypted, 16);                                       
        // Ciphertext (32:-16)
        EVP_DecryptUpdate(ctx, pOutput, &outlen, pEncrypted + 32, int(encryptedLength - EXTRA_BYTES));
        // Tag (-16:)
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, 
            const_cast<void*>(reinterpret_cast<const void*>((pEncrypted + encryptedLength - 16))));  

        if (outlen != encryptedLength - EXTRA_BYTES)
        {
            return false;
        }
        int status = EVP_DecryptFinal_ex(ctx, pOutput, &outlen);
        EVP_CIPHER_CTX_free(ctx);
        return status > 0;
    }
}
