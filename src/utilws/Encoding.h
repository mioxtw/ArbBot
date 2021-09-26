#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <string>

namespace utilws::encoding {

namespace {
struct HmacCtx
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
// code for version 1.1.0 or greater
    HMAC_CTX* ctx;
    HmacCtx() { 
        ctx = HMAC_CTX_new(); 
        HMAC_CTX_reset(ctx); 
    }
    ~HmacCtx() { HMAC_CTX_free(ctx); }
#else
// code for 1.0.x or lower
    HMAC_CTX ctx;
    HmacCtx() { HMAC_CTX_init(&ctx); }
    ~HmacCtx() { HMAC_CTX_cleanup(&ctx); }
#endif
};
}

std::string hmac(const std::string& secret,
                 std::string msg,
                 std::size_t signed_len)
{
    static HmacCtx hmac;
    char signed_msg[64];
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
// code for version 1.1.0 or greater
    HMAC_Init_ex(
      hmac.ctx, secret.data(), (int)secret.size(), EVP_sha256(), nullptr);
    HMAC_Update(hmac.ctx, (unsigned char*)msg.data(), msg.size());
    HMAC_Final(hmac.ctx, (unsigned char*)signed_msg, nullptr);
#else
// code for 1.0.x or lower
    HMAC_Init_ex(
      &hmac.ctx, secret.data(), (int)secret.size(), EVP_sha256(), nullptr);
    HMAC_Update(&hmac.ctx, (unsigned char*)msg.data(), msg.size());
    HMAC_Final(&hmac.ctx, (unsigned char*)signed_msg, nullptr);    
#endif
    return {signed_msg, signed_len};
}

namespace {
constexpr char hexmap[] = {'0',
                           '1',
                           '2',
                           '3',
                           '4',
                           '5',
                           '6',
                           '7',
                           '8',
                           '9',
                           'a',
                           'b',
                           'c',
                           'd',
                           'e',
                           'f'};
}

std::string hmac_string_to_hex(unsigned char* data, std::size_t len)
{
    std::string s(len * 2, ' ');
    for (std::size_t i = 0; i < len; ++i) {
        s[2 * i] = hexmap[(data[i] & 0xF0) >> 4];
        s[2 * i + 1] = hexmap[data[i] & 0x0F];
    }
    return s;
}
}
