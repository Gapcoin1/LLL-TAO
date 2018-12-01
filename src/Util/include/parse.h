/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2018

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#ifndef NEXUS_UTIL_INCLUDE_PARSE_H
#define NEXUS_UTIL_INCLUDE_PARSE_H

#include <vector>
#include <string>
#include <cstdint>

#include <LLC/types/uint1024.h>

#include <Util/include/debug.h>
#include <Util/include/convert.h>

/** ParseString
 *
 *  Parses a string and tokenizes it into substrings based on the character
 *  delemiter.
 *
 *  @param[in] str The string to parse.
 *
 *  @param[in] c The character delimiter.
 *
 *  @param[out] v The vector to store the tokens into.
 *
 **/
inline void ParseString(const std::string& str, char c, std::vector<std::string>& v)
{
    if (str.empty())
        return;

    std::string::size_type i1 = 0;
    std::string::size_type i2;
    {
        i2 = str.find(c, i1);
        if (i2 == str.npos)
        {
            v.push_back(str.substr(i1));

            return;
        }
        v.push_back(str.substr(i1, i2-i1));

        i1 = i2+1;
    }
}


/** FormatMoney
 *
 *  Take as input an encoded money amount and format it into an output string.
 *
 *  @param[in] n The encoded money amount
 *
 *  @param[in] fPlus Flag for if plus sign should be output for positive values.
 *
 *  @param[in] COIN_SIZE The magnitude of the coin amount used for decoding
 *                       decimal value from integer
 *
 *  @return The formatted money string.
 *
 **/
inline std::string FormatMoney(int64_t n, bool fPlus = false, int64_t COIN_SIZE = 1000000)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    int64_t n_abs = (n > 0 ? n : -n);
    int64_t quotient = n_abs / COIN_SIZE;
    int64_t remainder = n_abs % COIN_SIZE;

    std::string str = debug::strprintf("%" PRI64d ".%06" PRI64d "", quotient, remainder);

    // Right-trim excess 0's before the decimal point:
    int nTrim = 0;
    for (int i = str.size()-1; (str[i] == '0' && isdigit(str[i-2])); --i)
        ++nTrim;

    if (nTrim)
        str.erase(str.size()-nTrim, nTrim);

    if (n < 0)
        str.insert((uint32_t)0, 1, '-');
    else if (fPlus && n > 0)
        str.insert((uint32_t)0, 1, '+');
    return str;
}


/** ParseMoney
 *
 *  Parse the money amount from input string and return the encoded money value.
 *  Return if there were errors or not.
 *
 *  @param[in] str The input string to parse money amount from.
 *
 *  @param[out] nRet The amount encoded from the parsed money string
 *
 *  @return True if no errors, false otherwise.
 *
 **/
inline bool ParseMoney(const std::string& str, int64_t& nRet)
{
    return ParseMoney(str.c_str(), nRet);
}


/** ParseMoney
 *
 *  Parse the money amount from input string and return the encoded money value.
 *  Return if there were errors or not.
 *
 *  @param[in] pszIn The input string to parse money amount from.
 *
 *  @param[out] nRet The amount encoded from the parsed money string
 *
 *  @param[in] COIN_SIZE max amount of units of coins to encode
 *
 *  @param[in] CENT_SIZE max amount of units of cents to encode
 *
 *  @return True if no errors, false otherwise.
 *
 **/
inline bool ParseMoney(const char* pszIn, int64_t& nRet, int64_t COIN_SIZE = 1000000, int64_t CENT_SIZE = 10000)
{
    std::string strWhole;
    int64_t nUnits = 0;
    const char* p = pszIn;
    while (isspace(*p))
        p++;
    for (; *p; p++)
    {
        if (*p == '.')
        {
            p++;
            int64_t nMult = CENT_SIZE * 10;
            while (isdigit(*p) && (nMult > 0))
            {
                nUnits += nMult * (*p++ - '0');
                nMult /= 10;
            }

            break;
        }

        if (isspace(*p))
            break;

        if (!isdigit(*p))
            return false;

        strWhole.insert(strWhole.end(), *p);
    }

    for (; *p; p++)
        if (!isspace(*p))
            return false;
    if (strWhole.size() > 10) // guard against 63 bit overflow
        return false;
    if (nUnits < 0 || nUnits > COIN_SIZE)
        return false;

    int64_t nWhole = atoi64(strWhole);
    int64_t nValue = nWhole* COIN_SIZE + nUnits;

    nRet = nValue;
    return true;
}


/** Split
 *
 *  Split a string into it's components by delimiter.
 *
 *  @param[in] strInput The input string.
 *
 *  @param[in] strDelimiter The delimeter to seperate at.
 *
 *  @return The vector of the tokenized strings.
 *
 **/
inline std::vector<std::string> Split(const std::string& strInput, char strDelimiter)
{
    std::string::size_type nIndex = 0;
    std::string::size_type nFind  = strInput.find(strDelimiter);

    std::vector<std::string> vData;
    while (nFind != std::string::npos)
    {
        vData.push_back(strInput.substr(nIndex, nFind - nIndex));
        nIndex = ++ nFind;
        nFind  = strInput.find(strDelimiter, nFind);

        if (nFind == std::string::npos)
            vData.push_back(strInput.substr(nIndex, strInput.length()));
    }

    return vData;
}

#endif
