/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2021

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <TAO/API/types/commands/finance.h>

#include <LLD/include/global.h>

#include <TAO/API/include/build.h>
#include <TAO/API/include/format.h>
#include <TAO/API/include/list.h>

#include <TAO/API/types/authentication.h>
#include <TAO/API/types/commands.h>

#include <TAO/API/types/commands/names.h>
#include <TAO/API/types/commands/finance.h>

#include <TAO/Ledger/include/constants.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/state.h>

#include <TAO/Operation/include/enum.h>

#include <TAO/Register/include/create.h>
#include <TAO/Register/include/enum.h>
#include <TAO/Register/types/address.h>
#include <TAO/Register/types/object.h>

#include <Legacy/include/money.h>
#include <Legacy/types/address.h>
#include <Legacy/types/script.h>
#include <Legacy/wallet/wallet.h>
#include <Legacy/types/wallettx.h>

#include <map>
#include <string>
#include <vector>

/* Global TAO namespace. */
namespace TAO::API
{
    /* Migrate all Legacy wallet accounts to corresponding accounts in the signature chain */
    encoding::json Finance::MigrateAccounts(const encoding::json& jParams, const bool fHelp)
    {
        /* Return value array */
        encoding::json ret = encoding::json::array();

        #ifndef NO_WALLET

        Legacy::Wallet& wallet = Legacy::Wallet::Instance();

        /* Check for walletpassphrase parameter. */
        SecureString strWalletPass;
        strWalletPass.reserve(100);

        if(jParams.find("walletpassphrase") != jParams.end())
            strWalletPass = jParams["walletpassphrase"].get<std::string>().c_str();

        /* Check to see if the caller has specified NOT to create a name (we do by default) */
        bool fCreateName = jParams.find("createname") == jParams.end()
                || (jParams["createname"].get<std::string>() != "0" && jParams["createname"].get<std::string>() != "false");

        /* Save the current lock state of wallet */
        bool fLocked = wallet.IsLocked();
        bool fMintOnly = Legacy::fWalletUnlockMintOnly;

        /* Must provide passphrase to send if wallet locked or unlocked for minting only */
        if(wallet.IsCrypted() && (fLocked || fMintOnly))
        {
            if(strWalletPass.length() == 0)
                throw Exception(-179, "Legacy wallet is locked. walletpassphrase required");

            /* Unlock returns true if already unlocked, but passphrase must be validated for mint only so must lock first */
            if(fMintOnly)
            {
                wallet.Lock();
                Legacy::fWalletUnlockMintOnly = false; //Assures temporary unlock is a full unlock for send
            }

            /* Handle temporary unlock (send false for fStartStake so stake minter does not start during send)
             * An incorrect passphrase will leave the wallet locked, even if it was previously unlocked for minting.
             */
            if(!wallet.Unlock(strWalletPass, 0, false))
                throw Exception(-180, "Incorrect walletpassphrase for Legacy wallet");
        }

        /* Get a map of all account balances from the legacy wallet */
        std::map<std::string, int64_t> mapAccountBalances;
        for(const auto& entry : Legacy::Wallet::Instance().GetAddressBook().GetAddressBookMap())
        {
            if(Legacy::Wallet::Instance().HaveKey(entry.first)) // This address belongs to me
            {
                if(entry.second == "" || entry.second == "default")
                    mapAccountBalances["default"] = 0;
                else
                    mapAccountBalances[entry.second] = 0;
            }
        }

        /* Get the available addresses from the wallet */
        std::map<Legacy::NexusAddress, int64_t> mapAddresses;
        if(!Legacy::Wallet::Instance().GetAddressBook().AvailableAddresses((uint32_t)runtime::unifiedtimestamp(), mapAddresses))
            throw Exception(-3, "Error Extracting the Addresses from Wallet File. Please Try Again.");

        /* Find all the addresses in the list */
        for(const auto& entry : mapAddresses)
        {
            if(Legacy::Wallet::Instance().GetAddressBook().HasAddress(entry.first))
            {
                std::string strAccount = Legacy::Wallet::Instance().GetAddressBook().GetAddressBookMap().at(entry.first);

                /* Make sure to map blank legacy account to default */
                if(strAccount == "")
                    strAccount = "default";

                mapAccountBalances[strAccount] += entry.second;
            }
            else
            {
                mapAccountBalances["default"] += entry.second;
            }
        }


        /* map of legacy account names to tritium account register addresses */
        std::map<std::string, TAO::Register::Address> mapAccountRegisters;

        /* The PIN to be used for this API call */
        SecureString strPIN;

        /* Unlock grabbing the pin, while holding a new authentication lock */
        RECURSIVE(Authentication::Unlock(jParams, strPIN, TAO::Ledger::PinUnlock::TRANSACTIONS));

        /* Cache a copy of our genesis-id. */
        const uint256_t hashGenesis =
            Authentication::Caller(jParams);

        /* Iterate the legacy accounts */
        std::vector<TAO::Operation::Contract> vContracts;
        for(const auto& accountBalance :  mapAccountBalances)
        {
            /* The name of the legacy account */
            std::string strAccount = accountBalance.first;

            /* The new account address */
            TAO::Register::Address hashAccount;

            /* First check to see if an account exists with this name */
            hashAccount = Names::ResolveAddress(jParams, strAccount, false);

            /* If one does not exist then check to see if one exists with a matching data field, from a previous migration */
            if(!hashAccount.IsValid())
            {
                std::vector<TAO::Register::Address> vAccounts;
                if(ListAccounts(hashGenesis, vAccounts, false, false))
                {
                    for(const auto& hashRegister : vAccounts)
                    {
                        /* Retrieve the account */
                        TAO::Register::Object object;
                        if(!LLD::Register->ReadState(hashRegister, object, TAO::Ledger::FLAGS::MEMPOOL))
                            throw TAO::API::Exception(-13, "Object not found");

                        /* Parse the object register. */
                        if(!object.Parse())
                            throw TAO::API::Exception(-14, "Object failed to parse");

                        /* Check to see if it is a NXS account the data matches the account name */
                        if(object.get<uint256_t>("token") == 0
                            && object.Check("data") && object.get<std::string>("data") == strAccount)
                        {
                            hashAccount = hashRegister;
                            break;
                        }
                    }
                }
            }

            /* If we still haven't found an account then create a new one */
            if(!hashAccount.IsValid())
            {
                /* Generate a random hash for this objects register address */
                hashAccount = TAO::Register::Address(TAO::Register::Address::ACCOUNT);

                /* Create an account object register for NXS (identifier 0). */
                TAO::Register::Object tAccount =
                    TAO::Register::CreateAccount(0);

                /* Store the legacy account name in the data field. */
                tAccount << std::string("data") << uint8_t(TAO::Register::TYPES::STRING) << strAccount;

                /* Submit the payload object. */
                TAO::Operation::Contract tContract;
                tContract << uint8_t(TAO::Operation::OP::CREATE)      << hashAccount;
                tContract << uint8_t(TAO::Register::REGISTER::OBJECT) << tAccount.GetState();

                /* Add this contract to our payload. */
                vContracts.push_back(tContract);

                /* If user has not explicitly indicated not to create a name then create a Name Object register for it. */
                if(fCreateName)
                    BuildName(jParams, hashAccount, vContracts);
            }

            /* Add this to the map */
            mapAccountRegisters[strAccount] = hashAccount;
        }

        /* If there are accounts to create then submit the transaction */
        if(!vContracts.empty())
            BuildAndAccept(jParams, vContracts, TAO::Ledger::PinUnlock::UnlockActions::TRANSACTIONS);

        /* Once the accounts have been created transfer the balance from the legacy account to the new ones */
        for(const auto& accountBalance :  mapAccountBalances)
        {
            /* Check that there is enough balance to send */
            if(accountBalance.second <= Legacy::TRANSACTION_FEE)
                continue;

            /* The account to send from */
            std::string strAccount = accountBalance.first;

            /* The account address to send to */
            TAO::Register::Address hashAccount = mapAccountRegisters[strAccount];

            /* The amount to send */
            int64_t nAmount = accountBalance.second;

            /* The script to contain the recipient */
            Legacy::Script scriptPubKey;
            scriptPubKey.SetRegisterAddress(hashAccount);

            /* Legacy wallet transaction  */
            Legacy::WalletTx wtx;

            /* Set the from account */
            wtx.strFromAccount = strAccount;

            /* Create the legacy transaction */
            std::string strException = wallet.SendToNexusAddress(scriptPubKey, nAmount, wtx, false, 1, true);

            encoding::json entry;
            entry["account"] = strAccount;
            entry["address"] = hashAccount.ToString();
            entry["amount"]  = FormatBalance(nAmount);

            if(!strException.empty())
                entry["error"] = strException;
            else
                entry["txid"] = wtx.GetHash().GetHex();

            ret.push_back(entry);
        }

        /* If used walletpassphrase to temporarily unlock wallet, re-lock the wallet
         * This does not return unlocked for minting state, because we are migrating from the trust key and
         * the minter should not be re-started.
         */
        if(wallet.IsCrypted() && (fLocked || fMintOnly))
            wallet.Lock();

        #endif

        return ret;
    }
}
