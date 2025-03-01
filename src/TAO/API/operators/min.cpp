/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/exception.h>
#include <TAO/API/types/operators/array.h>
#include <TAO/API/types/operators/min.h>

#include <Util/types/precision.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Operators::Min(const encoding::json& jParams, const encoding::json& jResult)
    {
        /* Build our array object. */
        const encoding::json jArray =
            Operators::Array(jParams, jResult);

        /* Check for empty results. */
        if(jArray.empty())
            throw Exception(-123, "Operator [min] cannot be used on empty result");

        /* Loop through our entries to find minimum value. */
        encoding::json jRet = { {"min", jArray[0]} };
        for(uint32_t n = 1; n < jArray.size(); ++n)
        {
            /* Handle for unsigned signed integers. */
            if(jArray[n].is_number_unsigned())
            {
                /* Grab our values. */
                const uint64_t nValue = jArray[n].get<uint64_t>();

                /* Check if below minimum value. */
                if(nValue < jRet["min"].get<uint64_t>())
                    jRet["min"] = nValue;
            }

            /* Handle for signed integers. */
            if(jArray[n].is_number_integer())
            {
                /* Grab our values. */
                const int64_t nValue = jArray[n].get<int64_t>();

                /* Check if below minimum value. */
                if(nValue < jRet["min"].get<int64_t>())
                    jRet["min"] = nValue;
            }

            /* Handle for floats. */
            if(jArray[n].is_number_float())
            {
                /* Grab our values. */
                const precision_t dValue =
                    precision_t(jArray[n].dump());

                /* Check if above maximum value. */
                if(dValue > precision_t(jRet["min"].dump()))
                    jRet["min"] = dValue.double_t();
            }
        }

        return jRet;
    }
}
