/*__________________________________________________________________________________________

            (c) Hash(BEGIN(Satoshi[2010]), END(Sunny[2012])) == Videlicet[2014] ++

            (c) Copyright The Nexus Developers 2014 - 2019

            Distributed under the MIT software license, see the accompanying
            file COPYING or http://www.opensource.org/licenses/mit-license.php.

            "ad vocem populi" - To the Voice of the People

____________________________________________________________________________________________*/

#include <LLD/include/global.h>

#include <TAO/API/include/execute.h>

#include <TAO/Register/include/unpack.h>

#include <TAO/Ledger/types/merkle.h>
#include <TAO/Ledger/types/state.h>
#include <TAO/Ledger/types/client.h>

/* Global TAO namespace. */
namespace TAO
{

    /* Ledger Layer namespace. */
    namespace Ledger
    {

        /* The default constructor. */
        MerkleTx::MerkleTx()
        : Transaction   ( )
        , hashBlock     (0)
        , vMerkleBranch ( )
        , nIndex        (0)
        {
        }


        /* Copy constructor. */
        MerkleTx::MerkleTx(const MerkleTx& tx)
        : Transaction   (tx)
        , hashBlock     (tx.hashBlock)
        , vMerkleBranch (tx.vMerkleBranch)
        , nIndex        (tx.nIndex)
        {
        }


        /* Move constructor. */
        MerkleTx::MerkleTx(MerkleTx&& tx) noexcept
        : Transaction   (std::move(tx))
        , hashBlock     (std::move(tx.hashBlock))
        , vMerkleBranch (std::move(tx.vMerkleBranch))
        , nIndex        (std::move(tx.nIndex))
        {
        }


        /* Copy constructor. */
        MerkleTx::MerkleTx(const Transaction& tx)
        : Transaction   (tx)
        , hashBlock     (0)
        , vMerkleBranch ( )
        , nIndex        (0)
        {
        }


        /* Move constructor. */
        MerkleTx::MerkleTx(Transaction&& tx) noexcept
        : Transaction   (std::move(tx))
        , hashBlock     (0)
        , vMerkleBranch ( )
        , nIndex        (0)
        {
        }


        /* Copy assignment. */
        MerkleTx& MerkleTx::operator=(const MerkleTx& tx)
        {
            vContracts    = tx.vContracts;
            nVersion      = tx.nVersion;
            nSequence     = tx.nSequence;
            nTimestamp    = tx.nTimestamp;
            hashNext      = tx.hashNext;
            hashRecovery  = tx.hashRecovery;
            hashGenesis   = tx.hashGenesis;
            hashPrevTx    = tx.hashPrevTx;
            nKeyType      = tx.nKeyType;
            nNextType     = tx.nNextType;
            vchPubKey     = tx.vchPubKey;
            vchSig        = tx.vchSig;
            hashCache     = tx.hashCache;

            hashBlock     = tx.hashBlock;
            vMerkleBranch = tx.vMerkleBranch;
            nIndex        = tx.nIndex;

            return *this;
        }


        /* Move assignment. */
        MerkleTx& MerkleTx::operator=(MerkleTx&& tx) noexcept
        {
            vContracts    = std::move(tx.vContracts);
            nVersion      = std::move(tx.nVersion);
            nSequence     = std::move(tx.nSequence);
            nTimestamp    = std::move(tx.nTimestamp);
            hashNext      = std::move(tx.hashNext);
            hashRecovery  = std::move(tx.hashRecovery);
            hashGenesis   = std::move(tx.hashGenesis);
            hashPrevTx    = std::move(tx.hashPrevTx);
            nKeyType      = std::move(tx.nKeyType);
            nNextType     = std::move(tx.nNextType);
            vchPubKey     = std::move(tx.vchPubKey);
            vchSig        = std::move(tx.vchSig);
            hashCache     = std::move(tx.hashCache);

            hashBlock     = std::move(tx.hashBlock);
            vMerkleBranch = std::move(tx.vMerkleBranch);
            nIndex        = std::move(tx.nIndex);

            return *this;
        }


        /* Copy assignment. */
        MerkleTx& MerkleTx::operator=(const Transaction& tx)
        {
            vContracts    = tx.vContracts;
            nVersion      = tx.nVersion;
            nSequence     = tx.nSequence;
            nTimestamp    = tx.nTimestamp;
            hashNext      = tx.hashNext;
            hashRecovery  = tx.hashRecovery;
            hashGenesis   = tx.hashGenesis;
            hashPrevTx    = tx.hashPrevTx;
            nKeyType      = tx.nKeyType;
            nNextType     = tx.nNextType;
            vchPubKey     = tx.vchPubKey;
            vchSig        = tx.vchSig;
            hashCache     = tx.hashCache;

            return *this;
        }


        /* Move assignment. */
        MerkleTx& MerkleTx::operator=(Transaction&& tx) noexcept
        {
            vContracts    = std::move(tx.vContracts);
            nVersion      = std::move(tx.nVersion);
            nSequence     = std::move(tx.nSequence);
            nTimestamp    = std::move(tx.nTimestamp);
            hashNext      = std::move(tx.hashNext);
            hashRecovery  = std::move(tx.hashRecovery);
            hashGenesis   = std::move(tx.hashGenesis);
            hashPrevTx    = std::move(tx.hashPrevTx);
            nKeyType      = std::move(tx.nKeyType);
            nNextType     = std::move(tx.nNextType);
            vchPubKey     = std::move(tx.vchPubKey);
            vchSig        = std::move(tx.vchSig);
            hashCache     = std::move(tx.hashCache);

            return *this;
        }


        /* Default Destructor */
        MerkleTx::~MerkleTx()
        {
        }


        /* Checks if this transaction has a valid merkle path.*/
        bool MerkleTx::CheckMerkleBranch(const uint512_t& hashMerkleRoot) const
        {
            /* Generate merkle root from merkle branch. */
            const uint512_t hashMerkleCheck =
                Block::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex);

            return hashMerkleRoot == hashMerkleCheck;
        }


        /* Builds a merkle branch from block state. */
        bool MerkleTx::BuildMerkleBranch(const BlockState& state)
        {
            /* Cache this txid. */
            uint512_t hash = GetHash();

            /* Find the index of this transaction. */
            for(nIndex = 0; nIndex < state.vtx.size(); ++nIndex)
                if(state.vtx[nIndex].second == hash)
                    break;

            /* Check for valid index. */
            if(nIndex == state.vtx.size())
                return false;

            /* Build merkle branch. */
            vMerkleBranch = state.GetMerkleBranch(state.vtx, nIndex);

            return true;
        }


        /* Builds a merkle branch from block state. */
        bool MerkleTx::BuildMerkleBranch(const uint1024_t& hashConfirmed)
        {
            /* Get the confirming block. */
            TAO::Ledger::BlockState state;
            if(!LLD::Ledger->ReadBlock(hashConfirmed, state))
                return debug::error(FUNCTION, "no valid block to generate merkle path");

            /* Set his block's hash. */
            hashBlock     = hashConfirmed;

            return BuildMerkleBranch(state);
        }


        /* Builds a merkle branch without any block data. */
        bool MerkleTx::BuildMerkleBranch()
        {
            /* Cache this txid. */
            uint512_t hash = GetHash();

            /* Get the confirming block. */
            TAO::Ledger::BlockState state;
            if(!LLD::Ledger->ReadBlock(hash, state))
                return false;

            /* Set his block's hash. */
            hashBlock = state.GetHash();

            return BuildMerkleBranch(state);
        }


        /* Commits a merkle transaction to lookup internal memory. */
        bool MerkleTx::CommitLookup(const uint256_t& hashRegister)
        {
            /* Reverse iterate contracts and terminate when we found the register. */
            for(auto pContract = vContracts.rbegin(); pContract != vContracts.rend(); ++pContract)
            {
                /* Bind our contract to this transaction. */
                pContract->Bind(this);

                /* Unpack the address we will be working on. */
                uint256_t hashAddress;
                if(!TAO::Register::Unpack(*pContract, hashAddress))
                    continue;

                /* Check that our register addresses match. */
                if(hashRegister != hashAddress)
                    continue;

                /* Get our register post-state now. */
                const TAO::Register::Object tRegister =
                    TAO::API::ExecuteContract(*pContract);

                /* Commit our register to disk. */
                return LLD::Register->WriteState(hashRegister, tRegister, TAO::Ledger::FLAGS::LOOKUP);
            }

            return false;
        }

        /* Verifies a merkle transaction against the block merkle root and internal checks. */
        bool MerkleTx::Verify() const
        {
            /* Check for empty merkle tx. */
            if(hashBlock == 0)
                return debug::error(FUNCTION, "block to compare merkle branch");

            /* Check transaction contains valid information. */
            if(!Check())
                return debug::error(FUNCTION, debug::GetLastError());

            /* Grab the block to check merkle path. */
            TAO::Ledger::ClientBlock block;
            if(!LLD::Client->ReadBlock(hashBlock, block))
                return debug::error(FUNCTION, "missing block ", hashBlock.SubString());

            /* Check the merkle branch. */
            if(!CheckMerkleBranch(block.hashMerkleRoot))
                return debug::error(FUNCTION, "merkle transaction has invalid path");

            return true;
        }
    }
}
