/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/users/types/users.h>
#include <TAO/API/types/session-manager.h>
#include <TAO/Ledger/types/stake_minter.h>

#include <LLD/include/global.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        /* Lock an account for mining (TODO: make this much more secure) */
        encoding::json Users::Lock(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* Get the session */
            Session& session = GetSession(params);

            /* Check if already unlocked. */
            if(session.GetActivePIN() == nullptr || (!session.GetActivePIN() && session.GetActivePIN()->PIN() == ""))
                throw Exception(-132, "Account already locked");

            /* The current unlock actions */
            uint8_t nUnlockedActions = session.GetActivePIN()->UnlockedActions();

            /* Check for mining flag. */
            if(params.find("mining") != params.end())
            {
                std::string strMint = params["mining"].get<std::string>();

                if(strMint == "1" || strMint == "true")
                {
                     /* Check if already locked. */
                    if(session.GetActivePIN() != nullptr && !session.GetActivePIN()->CanMine())
                        throw Exception(-196, "Account already locked for mining");
                    else
                        nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::MINING;
                }
            }

            /* Check for staking flag. */
            if(params.find("staking") != params.end())
            {
                std::string strMint = params["staking"].get<std::string>();

                if(strMint == "1" || strMint == "true")
                {
                     /* Check if already locked. */
                    if(session.GetActivePIN() != nullptr && !session.GetActivePIN()->CanStake())
                        throw Exception(-197, "Account already locked for staking");
                    else
                        nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::STAKING;
                }
            }

            /* Check transactions flag. */
            if(params.find("transactions") != params.end())
            {
                std::string strTransactions = params["transactions"].get<std::string>();

                if(strTransactions == "1" || strTransactions == "true")
                {
                     /* Check if already unlocked. */
                    if(session.GetActivePIN() != nullptr && !session.GetActivePIN()->CanTransact())
                        throw Exception(-198, "Account already locked for transactions");
                    else
                        nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS;
                }
            }

            /* Check for notifications. */
            if(params.find("notifications") != params.end())
            {
                std::string strNotifications = params["notifications"].get<std::string>();

                if(strNotifications == "1" || strNotifications == "true")
                {
                     /* Check if already unlocked. */
                    if(session.GetActivePIN() != nullptr && !session.GetActivePIN()->ProcessNotifications())
                        throw Exception(-199, "Account already locked for notifications");
                    else
                        nUnlockedActions &= ~TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS;
                }
            }

            /* Cache the pin here before we clear it so that we an use it for saving the session */
            SecureString strPin = session.GetActivePIN()->PIN();

            /* Clear the pin */
            /* If we have changed specific unlocked actions them set them on the pin */
            if(nUnlockedActions != session.GetActivePIN()->UnlockedActions())
            {
                /* Set new unlock options */
                session.UpdatePIN(strPin, nUnlockedActions);
            }
            else
            {
                /* If no unlock actions left then free the pin from cache */
                session.ClearActivePIN();
            }

            /* Stop the stake minter if it is no longer unlocked for staking */
            if(session.GetActivePIN() != nullptr || (!(nUnlockedActions & TAO::Ledger::PinUnlock::UnlockActions::STAKING)))
            {
                /* If stake minter is running, stop it */
                TAO::Ledger::StakeMinter& stakeMinter = TAO::Ledger::StakeMinter::GetInstance();
                if(stakeMinter.IsStarted())
                    stakeMinter.Stop();
            }

            /* Update the saved session if there is one */
            if(LLD::Local->HasSession(session.GetAccount()->Genesis()))
                session.Save(strPin);

            /* populate unlocked status */
            encoding::json jsonUnlocked;

            jsonUnlocked["mining"] =
                (session.GetActivePIN() != nullptr && session.GetActivePIN()->CanMine());

            jsonUnlocked["notifications"] =
                (session.GetActivePIN() != nullptr && session.GetActivePIN()->ProcessNotifications());

            jsonUnlocked["staking"] =
                (session.GetActivePIN() != nullptr && session.GetActivePIN()->CanStake());

            jsonUnlocked["transactions"] =
                (session.GetActivePIN() != nullptr && session.GetActivePIN()->CanTransact());

            ret["unlocked"] = jsonUnlocked;
            return ret;
        }
    }
}
