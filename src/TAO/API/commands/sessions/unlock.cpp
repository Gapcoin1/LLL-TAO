/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/


#include <TAO/API/include/extract.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands/sessions.h>

#include <TAO/Ledger/types/stake_minter.h>

#include <Util/include/allocators.h>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Unlock an account for any given action. */
    encoding::json Sessions::Unlock(const encoding::json& jParams, const bool fHelp)
    {
        /* Get our current unlocked status. */
        uint8_t nCurrentActions = TAO::Ledger::PinUnlock::UnlockActions::NONE; // default to NO actions
        Authentication::UnlockStatus(jParams, nCurrentActions);

        /* Check for unlock actions */
        uint8_t nUnlockedActions = nCurrentActions;

        /* Check for mining flag. */
        if(ExtractBoolean(jParams, "mining"))
        {
            /* Can't unlock for mining in multiuser mode */
            if(config::fMultiuser.load())
                throw Exception(-288, "Cannot unlock for mining in multiuser mode");

            /* Adjust the unlocked flags. */
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::MINING;
        }

        /* Check for staking flag. */
        if(ExtractBoolean(jParams, "staking"))
        {
            /* Can't unlock for staking in multiuser mode */
            if(config::fMultiuser.load())
                throw Exception(-289, "Cannot unlock for staking in multiuser mode");

            /* Adjust the unlocked flags. */
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::STAKING;
        }

        /* Check transactions flag. */
        if(ExtractBoolean(jParams, "transactions"))
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;

        /* Check for notifications. */
        if(ExtractBoolean(jParams, "notifications"))
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS;

        /* If no unlock actions have been specifically set then default it to all */
        if(ExtractBoolean(jParams, "all"))
            nUnlockedActions |= TAO::Ledger::PinUnlock::UnlockActions::ALL;

        /* Check for no actions. */
        if(nUnlockedActions != nCurrentActions)
        {
            /* Check for authenticated sigchain. */
            if(!Authentication::Authenticate(jParams))
                throw Exception(-333, "Account failed to authenticate");

            /* Update our session with new pin. */
            Authentication::Update(jParams, nUnlockedActions);

            /* After unlock complete, attempt to start stake minter if unlocked for staking */
            if(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::STAKING)
            {
                /* Grab a reference of our stake minter. */
                TAO::Ledger::StakeMinter& rStakeMinter =
                    TAO::Ledger::StakeMinter::GetInstance();

                /* Start it if not started. */
                if(!rStakeMinter.IsStarted())
                    rStakeMinter.Start();
            }
        }

        /* Populate unlocked status */
        const encoding::json jRet =
        {
            {
                "unlocked",
                {
                    { "mining",        bool(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::MINING        )},
                    { "notifications", bool(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS )},
                    { "staking",       bool(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::STAKING       )},
                    { "transactions",  bool(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS  )}
                }
            }
        };

        return jRet;
    }
}
