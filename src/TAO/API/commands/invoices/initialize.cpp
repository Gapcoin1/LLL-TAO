/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/invoices.h>
#include <TAO/API/types/commands/templates.h>

#include <TAO/API/include/check.h>
#include <TAO/API/include/constants.h>
#include <TAO/API/include/json.h>

#include <TAO/Ledger/include/enum.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Standard initialization function. */
    void Invoices::Initialize()
    {
        /* Populate our invoice standard. */
        mapStandards["invoice"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [](const TAO::Register::Object& objCheck)
            {
                /* Check for correct state type. */
                if(objCheck.nType != TAO::Register::REGISTER::READONLY)
                    return false;

                /* Reset read position. */
                objCheck.nReadPos = 0;

                /* Find our leading type byte. */
                uint16_t nType;
                objCheck >> nType;

                /* Cleanup our read position. */
                objCheck.nReadPos = 0;

                /* Check that this matches our user type. */
                if(nType != USER_TYPES::INVOICE)
                    return false;

                return true;
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Invoices::InvoiceToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Subset of invoice standard, to find outstanding invoices. */
        mapStandards["outstanding"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& objCheck)
            {
                /* Check for correct state type. */
                if(!CheckObject("invoice", objCheck))
                    return false;

                return (objCheck.hashOwner.GetType() == TAO::Ledger::GENESIS::SYSTEM);
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Invoices::InvoiceToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Subset of invoice standard, to find outstanding invoices. */
        mapStandards["paid"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& objCheck)
            {
                /* Check for correct state type. */
                if(!CheckObject("invoice", objCheck))
                    return false;

                /* Get our recipient. */
                encoding::json jInvoice = RegisterToJSON(objCheck);
                if(jInvoice.find("json") == jInvoice.end())
                    return false;

                /* Check for recipient now. */
                if(jInvoice["json"].find("recipient") == jInvoice["json"].end())
                    return false;

                return (objCheck.hashOwner == uint256_t(jInvoice["json"]["recipient"].get<std::string>()));
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Invoices::InvoiceToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );

        /* Subset of invoice standard, to find outstanding invoices. */
        mapStandards["cancelled"] = Standard
        (
            /* Lambda expression to determine object standard. */
            [this](const TAO::Register::Object& objCheck)
            {
                /* Check for correct state type. */
                if(!CheckObject("invoice", objCheck))
                    return false;

                /* Get our recipient. */
                encoding::json jInvoice = RegisterToJSON(objCheck);
                if(jInvoice.find("json") == jInvoice.end())
                    return false;

                /* Check for recipient now. */
                if(jInvoice["json"].find("recipient") == jInvoice["json"].end())
                    return false;

                return (objCheck.hashOwner != uint256_t(jInvoice["json"]["recipient"].get<std::string>()));
            }

            /* Our custom encoding function for this type. */
            , std::bind
            (
                &Invoices::InvoiceToJSON,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );




        /* Handle for all CREATE operations. */
        mapFunctions["create"] = Function
        (
            std::bind
            (
                &Invoices::Create,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , TAO::Ledger::StartTransactionTimelock(2)
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
            , TAO::Ledger::StartTransactionTimelock(2)
        );

        /* Handle for all PAY operations. */
        mapFunctions["pay"] = Function
        (
            std::bind
            (
                &Invoices::Pay,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , TAO::Ledger::StartTransactionTimelock(2)
        );

        /* Handle for all CANCEL operations. */
        mapFunctions["cancel"] = Function
        (
            std::bind
            (
                &Invoices::Cancel,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , TAO::Ledger::StartTransactionTimelock(2)
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
            , TAO::Ledger::StartTransactionTimelock(2)
        );

        /* Handle for all LIST operations. */
        mapFunctions["list/history"] = Function
        (
            std::bind
            (
                &Invoices::History,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
            , TAO::Ledger::StartTransactionTimelock(2)
        );
    }
}
