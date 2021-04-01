/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#pragma once
#ifndef NEXUS_LLP_INCLUDE_GLOBAL_H
#define NEXUS_LLP_INCLUDE_GLOBAL_H

#include <LLP/include/port.h>
#include <LLP/types/tritium.h>
#include <LLP/types/time.h>
#include <LLP/templates/server.h>

#include <LLP/types/apinode.h>
#include <LLP/types/rpcnode.h>
#include <LLP/types/miner.h>
#include <LLP/types/p2p.h>

namespace LLP
{
    extern Server<TritiumNode>*  TRITIUM_SERVER;
    extern Server<TimeNode>*     TIME_SERVER;
    extern Server<APINode>*      API_SERVER;
    extern Server<RPCNode>*      RPC_SERVER;
    extern Server<Miner>*        MINING_SERVER;
    extern Server<P2PNode>*      P2P_SERVER;


    /** Current session identifier. **/
    const extern uint64_t SESSION_ID;


    /** Initialize
     *
     *  Initialize the LLP.
     *
     **/
    bool Initialize();


    /** Shutdown
     *
     *  Shutdown the LLP.
     *
     **/
    void Shutdown();


    /** MakeConnections
     *
     *  Makes connections from -connect and -addnode arguments for the specified server.
     *
     *  @param[in] pServer The server to make connections with.
     *
     **/
    template <class ProtocolType>
    void MakeConnections(Server<ProtocolType> *pServer)
    {
        /* -connect means try to establish a connection first. */
        if(config::mapMultiArgs["-connect"].size() > 0)
        {
            /* Add connections and resolve potential DNS lookups. */
            for(const auto& address : config::mapMultiArgs["-connect"])
            {
                /* Flag indicating connection was successful */
                bool fConnected = false;

                /* First attempt SSL if configured */
                if(pServer->SSLEnabled())
                   fConnected = pServer->AddConnection(address, pServer->GetPort(true), true, true);

                /* If SSL connection failed or was not attempted and SSL is not required, attempt on the non-SSL port */
                if(!fConnected && !pServer->SSLRequired())
                    fConnected = pServer->AddConnection(address, pServer->GetPort(false), false, true);
            }
        }

        /* -addnode means add to address manager and let it make connections. */
        if(config::mapMultiArgs["-addnode"].size() > 0)
        {
            /* Add nodes and resolve potential DNS lookups. */
            for(const auto& node : config::mapMultiArgs["-addnode"])
                pServer->AddNode(node, true);
        }
    }


    /** Shutdown
     *
     *  Performs a shutdown and cleanup of resources on a server if it exists.
     *
     *  pServer The potential server.
     *
     **/
    template <class ProtocolType>
    void Shutdown(Server<ProtocolType> *pServer)
    {
        if(pServer)
        {
            delete pServer;
            debug::log(0, FUNCTION, ProtocolType::Name());
        }
    }

}

#endif
