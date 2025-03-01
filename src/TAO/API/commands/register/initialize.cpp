/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/finance.h>
#include <TAO/API/types/commands/register.h>
#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/operators/initialize.h>

#include <TAO/API/types/commands.h>

#include <TAO/Register/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Register::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


        /* Populate our CRYPTO standard. */
        mapStandards["crypto"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.Standard() == TAO::Register::OBJECTS::CRYPTO;
            }
            , "crypto"
        );

        /* Populate our OBJECT standard. */
        mapStandards["object"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.nType == TAO::Register::REGISTER::OBJECT;
            }
        );

        /* Populate our RAW standard. */
        mapStandards["raw"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.nType == TAO::Register::REGISTER::RAW;
            }
            , "raw"
        );

        /* Populate our READONLY standard. */
        mapStandards["readonly"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.nType == TAO::Register::REGISTER::READONLY;
            }
            , "readonly"
        );

        /* Populate our ANY standard. */
        mapStandards["any"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return true;
            }
        );


        /* Handle for generic get operations. */
        mapFunctions["get"] = Function
        (
            std::bind
            (
                &Templates::Get,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for generic list operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Register::List,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for generic list operations. */
        mapFunctions["history"] = Function
        (
            std::bind
            (
                &Register::History,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Handle for generic list operations. */
        mapFunctions["transactions"] = Function
        (
            std::bind
            (
                &Register::Transactions,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    }
}
