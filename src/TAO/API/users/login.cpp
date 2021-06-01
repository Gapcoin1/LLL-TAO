/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLC/include/random.h>

#include <LLD/include/global.h>

#include <LLP/include/global.h>
#include <LLP/types/tritium.h>

#include <TAO/API/users/types/users.h>
#include <TAO/API/include/global.h>
#include <TAO/API/types/sessionmanager.h>

#include <TAO/Register/types/object.h>

#include <TAO/Ledger/include/chainstate.h>
#include <TAO/Ledger/include/create.h>
#include <TAO/Ledger/include/enum.h>

#include <TAO/Ledger/types/mempool.h>
#include <TAO/Ledger/types/sigchain.h>
#include <TAO/Ledger/types/tritium_minter.h>

#include <Util/include/allocators.h>

/* Global TAO namespace. */
namespace TAO
{

    /* API Layer namespace. */
    namespace API
    {

        //TODO: have the authorization system build a SHA256 hash and salt on the client side as the AUTH hash.

        /* Login to a user account. */
        encoding::json Users::Login(const encoding::json& params, const bool fHelp)
        {
            /* JSON return value. */
            encoding::json ret;

            /* Pin parameter. */
            SecureString strPin;

            /* Check for username parameter. */
            if(params.find("username") == params.end())
                throw APIException(-127, "Missing username");

            /* Parse out username. */
            SecureString strUser = SecureString(params["username"].get<std::string>().c_str());

            /* Check for username size. */
            if(strUser.size() == 0)
                throw APIException(-133, "Zero-length username");

            /* Check for password parameter. */
            if(params.find("password") == params.end())
                throw APIException(-128, "Missing password");

            /* Parse out password. */
            SecureString strPass = SecureString(params["password"].get<std::string>().c_str());

            /* Check for password size. */
            if(strPass.size() == 0)
                throw APIException(-134, "Zero-length password");

            /* Check for pin parameter. Parse the pin parameter. */
            if(params.find("pin") != params.end())
                strPin = SecureString(params["pin"].get<std::string>().c_str());
            else if(params.find("PIN") != params.end())
                strPin = SecureString(params["PIN"].get<std::string>().c_str());
            else
                throw APIException(-129, "Missing PIN");

            /* Check for pin size. */
            if(strPin.size() == 0)
                throw APIException(-135, "Zero-length PIN");

            /* Create a temp sig chain for checking credentials */
            TAO::Ledger::SignatureChain user(strUser, strPass);

            /* Get the genesis ID of the user logging in. */
            uint256_t hashGenesis = user.Genesis();

            /* Get the last Transaction for this sig chain to authenticate with. */
            TAO::Ledger::Transaction txPrev;

            /* Check for -client mode. */
            if(config::fClient.load())
            {
                /* If not using multiuser then check to see whether another user is already logged in */
                if(GetSessionManager().Has(0) && GetSessionManager().Get(0).GetAccount()->Genesis() != hashGenesis)
                {
                    throw APIException(-140, "CLIENT MODE: Already logged in with a different username.");
                }
                else if(GetSessionManager().Has(0))
                {
                    encoding::json ret;
                    ret["genesis"] = hashGenesis.ToString();

                    return ret;
                }

                if(TAO::Ledger::ChainState::Synchronizing())
                    throw APIException(-297, "Cannot log in while synchronizing");

                /* In order to authenticate the user, at a minimum we need a transaction from the users sig chain containing the most
                   up to date credentials.  To achieve this we first check to see if this is a new sig chain (the genesis will be in
                   the mempool).  If it is not, we can lookup the users Crypto object register (since this is updated
                   whenever the password/pin changes).  Reading the crypto register from the DB will force a remote lookup from a
                   peer if it is missing or expired.  This process avoids us having to download the entire sig chain before logging
                   in, so we can download it asynchronously after logging them in.  */

                /* First check to see if this is a new sig chain and the genesis is in the mempool */
                bool fNewSigchain = !LLD::Ledger->HasGenesis(hashGenesis) && TAO::Ledger::mempool.Has(hashGenesis);

                /* IF this is not a new sig chain, force a lookup of the crypto register */
                if(!fNewSigchain )
                {
                    /* The address of the crypto object register, which is deterministic based on the genesis */
                    TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

                    /* Read the crypto object register.  This will fail if the caller has provided an invalid username. */
                    TAO::Register::Object crypto;
                    if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::LOOKUP))
                        throw APIException(-139, "Invalid credentials");

                    /* Get the last transaction. */
                    uint512_t hashLast;
                    if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                        throw APIException(-138, "No previous transaction found");

                    /* Get previous transaction */
                    if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                        throw APIException(-138, "No previous transaction found");
                }
                /* If this is a new sig chain, get the genesis tx from mempool */
                else if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                {
                    throw APIException(-137, "Couldn't get transaction");
                }

            }
            else if(!LLD::Ledger->HasGenesis(hashGenesis))
            {
                /* If user genesis not in ledger, this will throw an exception. Just a matter of which one. */

                /* Check the memory pool and compare hashes. */
                if(!TAO::Ledger::mempool.Has(hashGenesis))
                {
                    /* Account doesn't exist returns invalid credentials */
                    throw APIException(-139, "Invalid credentials");
                }

                /* Dissallow mempool login on mainnet unless this node is runing in client mode.  This is because in client mode we
                   need to log in and authenticate with a peer in order to subscribe and receive sig chain transactions, including
                   the confirmation merkle TX. Without doing this we would never receive confirmation. */
                if(!config::fTestNet.load())
                {
                    /* After credentials verified, disallow login while in mempool and unconfirmed */
                    throw APIException(-222, "User create pending confirmation");
                }

                /* Testnet allows mempool login. Get the memory pool transaction. */
                else if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                {
                    throw APIException(-137, "Couldn't get transaction");
                }
            }
            /* If we haven't looked up the txprev at this point, read the last known tx */
            else
            {
                /* Get the last transaction. */
                uint512_t hashLast;
                if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");

                /* Get previous transaction */
                if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                    throw APIException(-138, "No previous transaction found");
            }

            /* Genesis Transaction. */
            TAO::Ledger::Transaction tx;
            tx.NextHash(user.Generate(txPrev.nSequence + 1, strPin), txPrev.nNextType);

            /* Check for consistency. */
            if(txPrev.hashNext != tx.hashNext)
            {
                throw APIException(-139, "Invalid credentials");
            }

            /* Check the sessions. */
            {
                auto session = GetSessionManager().mapSessions.begin();
                while(session != GetSessionManager().mapSessions.end())
                {
                    if(session->second.GetAccount()->Genesis() == hashGenesis)
                    {
                        ret["genesis"] = hashGenesis.ToString();
                        ret["session"] = session->first.ToString();

                        return ret;
                    }

                    /* increment iterator */
                    ++session;
                }


            }

            /* If not using multiuser then check to see whether another user is already logged in */
            if(!config::fMultiuser.load() && GetSessionManager().Has(0) && GetSessionManager().Get(0).GetAccount()->Genesis() != hashGenesis)
            {
                throw APIException(-140, "Already logged in with a different username.");
            }

            /* Create the new session */
            Session& session = GetSessionManager().Add(user, strPin);

            /* Cache the txid that was used to authenticate their login */
            session.hashAuth = txPrev.GetHash();

            /* Add the session to the notifications processor */
            if(NOTIFICATIONS_PROCESSOR)
                NOTIFICATIONS_PROCESSOR->Add(session.ID());

            /* If this user has previously logged in with a different session and saved it to the local DB, then purge it as that
               session is now no longer relevant */
            if(LLD::Local->HasSession(hashGenesis))
                LLD::Local->EraseSession(hashGenesis);

            ret["genesis"] = hashGenesis.ToString();

            ret["session"] = session.ID().ToString();


            /* If in client mode, download the users signature chain transactions asynchronously. */
            if(config::fClient.load())
            {
                std::thread([&]()
                {
                    DownloadSigChain(hashGenesis, true);
                }).detach();
            }

            return ret;
        }

        /* Automatically logs in the sig chain using the credentials configured in the config file.  Will also create the sig
        *  chain if it doesn't exist and configured with autocreate=1.
        *  When autocreate=1 this will log in the user while sig chain create is still in the mempool */
        void Users::auto_login()
        {
            /* Flag indicating that the auto login has successfully run.  Once it has run successfully once it will not run again
               for the lifespan of the application, to avoid auto-logging you back in if you intentionally log out. */
            static bool fAutoLoggedIn = false;
            try
            {
                /* If we haven't already logged in at least once, are configured for auto login, and are not currently logged in */
                if(!fAutoLoggedIn && config::GetBoolArg("-autologin") && !config::fMultiuser.load() && !LoggedIn())
                {
                    /* Keep a the credentials in secure allocated strings. */
                    SecureString strUsername = config::GetArg("-username", "").c_str();
                    SecureString strPassword = config::GetArg("-password", "").c_str();
                    SecureString strPin = config::GetArg("-pin", "").c_str();

                    /* Check we have user/pass/pin */
                    if(strUsername.empty() || strPassword.empty() || strPin.empty())
                        throw APIException(-203, "Autologin missing username/password/pin");

                    /* Create a temp sig chain for checking credentials */
                    TAO::Ledger::SignatureChain user(strUsername, strPassword);

                    /* Get the genesis ID. */
                    uint256_t hashGenesis = user.Genesis();

                    /* Get the last Transaction for this sig chain to authenticate with. */
                    TAO::Ledger::Transaction txPrev;

                    /* Check for -client mode. */
                    if(config::fClient.load())
                    {
                        /* In client mode, wait until we are synchronized before logging in */
                        if(TAO::Ledger::ChainState::Synchronizing())
                            throw APIException(-297, "Cannot log in while synchronizing");

                        /* In order to authenticate the user, at a minimum we need a transaction from the users sig chain containing the most
                        up to date credentials.  To achieve this we first check to see if this is a new sig chain (the genesis will be in
                        the mempool).  If it is not, we can lookup the users Crypto object register (since this is updated
                        whenever the password/pin changes).  Reading the crypto register from the DB will force a remote lookup from a
                        peer if it is missing or expired.  This process avoids us having to download the entire sig chain before logging
                        in, so we can download it asynchronously after logging them in.  */

                        /* First check to see if this is a new sig chain and the genesis is in the mempool */
                        bool fNewSigchain = !LLD::Ledger->HasGenesis(hashGenesis) && TAO::Ledger::mempool.Has(hashGenesis);

                        /* IF this is not a new sig chain, force a lookup of the crypto register */
                        if(!fNewSigchain )
                        {
                            /* The address of the crypto object register, which is deterministic based on the genesis */
                            TAO::Register::Address hashCrypto = TAO::Register::Address(std::string("crypto"), hashGenesis, TAO::Register::Address::CRYPTO);

                            /* Read the crypto object register */
                            TAO::Register::Object crypto;
                            if(!LLD::Register->ReadState(hashCrypto, crypto, TAO::Ledger::FLAGS::LOOKUP))
                                throw APIException(-259, "Could not read crypto object register");

                            /* Get the last transaction. */
                            uint512_t hashLast;
                            if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                                throw APIException(-138, "No previous transaction found");

                            /* Get previous transaction */
                            if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                                throw APIException(-138, "No previous transaction found");
                        }
                        /* If this is a new sig chain, get the genesis tx from mempool */
                        else if(!TAO::Ledger::mempool.Get(hashGenesis, txPrev))
                        {
                            throw APIException(-137, "Couldn't get transaction");
                        }
                    }

                    /* See if the sig chain exists */
                    else
                    {
                        if(!LLD::Ledger->HasGenesis(hashGenesis) && !TAO::Ledger::mempool.Has(hashGenesis))
                        {
                            /* If it doesn't exist then create it if configured to do so */
                            if(config::GetBoolArg("-autocreate"))
                            {
                                /* Testnet is considered local if no dns is being used or if using a private network */
                                bool fLocalTestnet = config::fTestNet.load()
                                    && (!config::GetBoolArg("-dns", true) || config::fHybrid.load());

                                /* Can only create user if synced and (if not local) have connections.
                                * Return without create/login if cannot create, yet. It will have to try again.
                                */
                                if(TAO::Ledger::ChainState::Synchronizing()
                                || (LLP::TRITIUM_SERVER->GetConnectionCount() == 0 && !fLocalTestnet))
                                {
                                    return;
                                }

                                /* The genesis transaction  */
                                TAO::Ledger::Transaction tx;

                                /* Create the sig chain genesis transaction */
                                create_sig_chain(strUsername, strPassword, strPin, tx);

                                /* Display that login was successful. */
                                debug::log(0, "Auto-Create Successful");
                            }
                            else
                                throw APIException(-203, "Autologin user not found");
                        }

                        /* Get the last transaction. */
                        uint512_t hashLast;
                        if(!LLD::Ledger->ReadLast(hashGenesis, hashLast, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            throw APIException(-138, "No previous transaction found");
                        }

                        /* Get previous transaction */
                        if(!LLD::Ledger->ReadTx(hashLast, txPrev, TAO::Ledger::FLAGS::MEMPOOL))
                        {
                            throw APIException(-138, "No previous transaction found");
                        }
                    }

                    /* Genesis Transaction. */
                    TAO::Ledger::Transaction tx;
                    tx.NextHash(user.Generate(txPrev.nSequence + 1, config::GetArg("-pin", "").c_str()), txPrev.nNextType);

                    /* Check the credentials match the previous tx. */
                    if(txPrev.hashNext != tx.hashNext)
                    {
                        throw APIException(-139, "Invalid credentials");
                    }

                    /* Create the new session */
                    Session& session = GetSessionManager().Add(user, strPin);

                    /* Cache the txid that was used to authenticate their login */
                    session.hashAuth = txPrev.GetHash();


                    /* The unlock actions to apply for autologin.  NOTE we do NOT unlock for transactions */
                    uint8_t nUnlockActions = TAO::Ledger::PinUnlock::UnlockActions::MINING
                                           | TAO::Ledger::PinUnlock::UnlockActions::NOTIFICATIONS
                                           | TAO::Ledger::PinUnlock::UnlockActions::STAKING;

                    /* Set account to unlocked. */
                    session.UpdatePIN(config::GetArg("-pin", "").c_str(), nUnlockActions);


                    /* Display that login was successful. */
                    debug::log(0, "Auto-Login Successful");

                    /* Set the flag so that we don't attempt to log in again */
                    fAutoLoggedIn = true;

                    /* If in client mode, download the users signature chain transactions asynchronously. */
                    if(config::fClient.load())
                    {
                        std::thread([&]()
                        {
                            DownloadSigChain(hashGenesis, true);
                        }).detach();
                    }

                    /* Add the session to the notifications processor */
                    if(NOTIFICATIONS_PROCESSOR)
                        NOTIFICATIONS_PROCESSOR->Add(session.ID());

                    /* Start the stake minter if successful login. */
                    TAO::Ledger::TritiumMinter::GetInstance().Start();

                    /* If this user has previously logged in with a different session and saved it to the local DB, then purge it as that
                       session is now no longer relevant */
                    if(LLD::Local->HasSession(hashGenesis))
                        LLD::Local->EraseSession(hashGenesis);
                }
            }
            catch(const APIException& e)
            {
                /* Log the error */
                debug::error(FUNCTION, e.what());

                /* Any exception is a login failure so remove the session if it exists */
                if(GetSessionManager().Has(0))
                    GetSessionManager().Remove(0);
            }

        }
    }
}
