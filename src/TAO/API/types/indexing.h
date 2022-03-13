/*__________________________________________________________________________________________

			(c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

			(c) Copyright The Nexus Developers 2014 - 2019

			Distributed under the MIT software license, see the accompanying
			file COPYING or http://www.opensource.org/licenses/mit-license.php.

			"ad vocem populi" - To The Voice of The People

____________________________________________________________________________________________*/

#pragma once

#include <LLC/types/uint1024.h>

#include <TAO/API/types/commands.h>

#include <Util/templates/singleton.h>
#include <Util/types/lock_unique_ptr.h>

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

/* Global TAO namespace. */
namespace TAO::API
{
    /** @class
     *
     *  This class is responsible for dispatching indexing events to the various API calls.
     *  These events are transactions that are unpacked into their according command-sets.
     *
     **/
    class Indexing
    {
        /** Queue to handle dispatch requests. **/
        static util::atomic::lock_unique_ptr<std::queue<uint512_t>> EVENTS_QUEUE;


        /** Thread for running dispatch. **/
        static std::thread EVENTS_THREAD;


        /** Condition variable to wake up the indexing thread. **/
        static std::condition_variable CONDITION;


        /** Set to track active indexing entries. **/
        static std::set<std::string> REGISTERED;


        /** Mutex around registration. **/
        static std::mutex MUTEX;


    public:

        /** Initialize
         *
         *  Initializes the current indexing systems.
         *
         **/
        static void Initialize();


        /** RefreshEvents
         *
         *  Checks current events against transaction history to ensure we are up to date.
         *
         **/
        static void RefreshEvents();


        /** Push
         *
         *  Index a new transaction to relay thread.
         *
         *  @param[in] hashTx The txid to dispatch indexing for.
         *
         **/
        static void Push(const uint512_t& hashTx);


        /** Register
         *
         *  Register a new command-set to indexing by class type.
         *
         **/
        template<typename Type>
        static void Register()
        {
            /* Grab a copy of our name. */
            const std::string strCommands = Type::Name();
            if(!Commands::Has(strCommands))
                return; //we just exit if already registered

            LOCK(MUTEX);
            REGISTERED.insert(strCommands);

            debug::log(0, FUNCTION, "Registered ", VARIABLE(strCommands));
        }


        /** Relay Thread
         *
         *  Handle indexing of all events for API.
         *
         **/
        static void Manager();


        /** Shutdown
         *
         *  Shuts down the current indexing systems.
         *
         **/
        static void Shutdown();
    };
}
