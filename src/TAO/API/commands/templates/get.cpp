/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/exception.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/check.h>
#include <TAO/API/include/extract.h>
#include <TAO/API/include/filter.h>
#include <TAO/API/include/json.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Get the data from a digital asset */
    encoding::json Templates::Get(const encoding::json& jParams, const bool fHelp)
    {
        /* Get the Register address. */
        const TAO::Register::Address hashRegister = ExtractAddress(jParams);

        /* Get the token / account object. */
        TAO::Register::Object tObject;
        if(!LLD::Register->ReadObject(hashRegister, tObject, TAO::Ledger::FLAGS::LOOKUP))
            throw Exception(-13, "Object not found");

        /* Now lets check our expected types match. */
        if(!CheckStandard(jParams, tObject))
            throw Exception(-49, "Unsupported type for name/address");

        /* Build our response object. */
        encoding::json jRet =
            StandardToJSON(jParams, tObject, hashRegister);

        /* Filter out our expected fieldnames if specified. */
        FilterFieldname(jParams, jRet);

        return jRet;
    }
}
