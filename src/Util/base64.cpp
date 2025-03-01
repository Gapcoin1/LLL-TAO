/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <Util/include/base64.h>
#include <cstring>

namespace encoding
{

    static const char *pbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    /* Take a raw byte buffer and a encode it into base 64 */
    std::string EncodeBase64(const uint8_t* pch, size_t len)
    {
        std::string strRet="";
        strRet.reserve((len+2)/3*4);

        int mode=0, left=0;
        const uint8_t *pchEnd = pch+len;

        while(pch<pchEnd)
        {
            int enc = *(pch++);
            switch (mode)
            {
                case 0: // we have no bits
                    strRet += pbase64[enc >> 2];
                    left = (enc & 3) << 4;
                    mode = 1;
                    break;

                case 1: // we have two bits
                    strRet += pbase64[left | (enc >> 4)];
                    left = (enc & 15) << 2;
                    mode = 2;
                    break;

                case 2: // we have four bits
                    strRet += pbase64[left | (enc >> 6)];
                    strRet += pbase64[enc & 63];
                    mode = 0;
                    break;
            }
        }

        if(mode)
        {
            strRet += pbase64[left];
            strRet += '=';
            if(mode == 1)
                strRet += '=';
        }

        return strRet;
    }

    /* Take a string and a encode it into base 64 */
    std::string EncodeBase64(const std::string& str)
    {
        return EncodeBase64((const uint8_t*)str.c_str(), str.size());
    }

    /* Take an encoded base 64 buffer and decode it into it's original message. */
    std::vector<uint8_t> DecodeBase64(const char* p, bool* pfInvalid)
    {
        static const int decode64_table[256] =
        {
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1,
            -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
            29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
            49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
        };

        if(pfInvalid)
            *pfInvalid = false;

        std::vector<uint8_t> vchRet;
        vchRet.reserve(strlen(p)*3/4);

        int mode = 0;
        int left = 0;

        while(1)
        {
            int dec = decode64_table[(uint8_t)*p];
            if(dec == -1) break;
            p++;
            switch (mode)
            {
                case 0: // we have no bits and get 6
                    left = dec;
                    mode = 1;
                    break;

                case 1: // we have 6 bits and keep 4
                    vchRet.push_back((left<<2) | (dec>>4));
                    left = dec & 15;
                    mode = 2;
                    break;

                case 2: // we have 4 bits and get 6, we keep 2
                    vchRet.push_back((left<<4) | (dec>>2));
                    left = dec & 3;
                    mode = 3;
                    break;

                case 3: // we have 2 bits and get 6
                    vchRet.push_back((left<<6) | dec);
                    mode = 0;
                    break;
            }
        }

        if(pfInvalid)
            switch (mode)
            {
                case 0: // 4n base64 characters processed: ok
                    break;

                case 1: // 4n+1 base64 character processed: impossible
                    *pfInvalid = true;
                    break;

                case 2: // 4n+2 base64 characters processed: require '=='
                    if(left || p[0] != '=' || p[1] != '=' || decode64_table[(uint8_t)p[2]] != -1)
                        *pfInvalid = true;
                    break;

                case 3: // 4n+3 base64 characters processed: require '='
                    if(left || p[0] != '=' || decode64_table[(uint8_t)p[1]] != -1)
                        *pfInvalid = true;
                    break;
            }

        return vchRet;
    }

    /* Take an encoded base 64 string and decode it into it's original message. */
    std::string DecodeBase64(const std::string& str)
    {
        std::vector<uint8_t> vchRet = DecodeBase64(str.c_str());
        return std::string((const char*)&vchRet[0], vchRet.size());
    }

}
