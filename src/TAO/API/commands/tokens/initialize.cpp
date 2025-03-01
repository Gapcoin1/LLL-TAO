/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/tokens.h>
#include <TAO/API/types/commands/finance.h>
#include <TAO/API/types/commands/templates.h>
#include <TAO/API/types/operators/initialize.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/global.h>

#include <TAO/Register/include/enum.h>

#include <Util/include/string.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Tokens::Initialize()
    {
        /* Populate our operators. */
        Operators::Initialize(mapOperators);


        /* Populate our ACCOUNT standard. */
        mapStandards["account"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.Standard() == TAO::Register::OBJECTS::ACCOUNT;
            }
        );

        /* Populate our TOKEN standard. */
        mapStandards["token"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& rObject)
            {
                return rObject.Standard() == TAO::Register::OBJECTS::TOKEN;
            }
        );


        /* Handle for all BURN operations. */
        mapFunctions["burn"] = Function
        (
            std::bind
            (
                &Finance::Burn,
                Commands::Instance<Finance>(),
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(6, 0, 0)
            , "please use finance/burn command instead"
        );

        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Templates::Create,
                std::placeholders::_1,
                std::placeholders::_2,

                "standard",
                USER_TYPES::STANDARD,
                "standard"
            )
            , version::get_version(6, 0, 0)
            , "please use finance/create command instead"
        );

        /* Handle for all CREDIT operations. */
        mapFunctions["credit"] = Function
        (
            std::bind
            (
                &Finance::Credit,
                Commands::Instance<Finance>(),
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(6, 0, 0)
            , "please use finance/credit command instead"
        );

        /* Handle for all DEBIT operations. */
        mapFunctions["debit"] = Function
        (
            std::bind
            (
                &Finance::Debit,
                Commands::Instance<Finance>(),
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(6, 0, 0)
            , "please use finance/debit command instead"
        );

        /* Handle for all GET operations. */
        mapFunctions["get"] = Function
        (
            std::bind
            (
                &Templates::Get,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(6, 0, 0)
            , "please use finance/get command instead"
        );

        /* Handle for all LIST operations. */
        mapFunctions["list"] = Function
        (
            std::bind
            (
                &Templates::List,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(6, 0, 0)
            , "please use finance/list command instead"
        );

        /* List token transactions method. */
        mapFunctions["list/token/transactions"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use finance/transactions/token command instead"
        );

        /* List accounts for given token. */
        mapFunctions["list/token/accounts"] = Function
        (
            std::bind
            (
                &Tokens::ListTokenAccounts,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(6, 0, 0)
            , "please use register/list/accounts WHERE results.token=<token> command instead"
        );


        /* DEPRECATED */
        mapFunctions["list/account/transactions"] = Function
        (
            std::bind
            (
                &Templates::Deprecated,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , version::get_version(5, 1, 0)
            , "please use finance/transactions/account command instead"
        );
    }
}
