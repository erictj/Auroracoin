// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

#include <stdint.h>

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to be slower.
    static const double SIGCHECK_VERIFICATION_FACTOR = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64_t nTimeLastCheckpoint;
        int64_t nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    bool fEnabled = true;

    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (     0, uint256("0x2a8e100939494904af825b488596ddd536b3a96226ad02e0f7ab7ae472b27a8e"))
        (     1, uint256("0xf54c0f8ed0b8ba85f99525d37e7cc9a5107bd752a54d8778d6cfb4f36cb51131"))
        (     2, uint256("0x2e739d971f02265b83895c04854fcb4deb48806126097b5feaf92ffd4d2341d6"))
        (   123, uint256("0x76b2378c0cd904584d9c226d9ef7a4a91a4ed701f2da36e4bd486d0c7a27b1fd"))
        (  5810, uint256("0x71517f8219449fd56ade24c888bbfd7d228c898d2aac8a078fd655be4182e813"))
        (  6350, uint256("0x76afd9f23e61b513e0c5224754943a1b1a6ddbed73586416363095808ac12eb1"))
        ( 19849, uint256("0xe6708808d1fa5b187345c92931737995c5bc41ca7fddbbc7bd90ee905029799e"))
        ( 42396, uint256("0x86d59ee30e3fc01ef9f51394e6d8efc271e5efa383a4f4d39b4d1a0dd1ee1934"))
        ( 94979, uint256("0xf07ff3f01f2eac70a1068843d341640013d4f7e4c8987b9b8c873914a3093426"))
        (111689, uint256("0x0808656fd09d52260cc96c891595ee2739dd96440d1c2e1e670b2063f0c4133c"))
        (135631, uint256("0xb15221d956ce4fbeed1ca974fccd12ee1d89a1810ae409be1118dc1b73985d12"))
        (154582, uint256("0x5a6c6b21f3a10fac0b28768e5dc38dd0432e868a22a23650f37d170790a4e7e7"))
        (187609, uint256("0xf28d78c7af8ef0564ca2d4685c8fe4d316fab3602c95f97773ab04cc488aa457"))
        (201541, uint256("0x79338645e77ed3187795c39ff19c7e7f169315f8d2e70406f7e1843a1797d29e"))
        ;

    static const CCheckpointData data = {
        &mapCheckpoints,
        1455154758, // * UNIX timestamp of last checkpoint block
        602320,   // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        60.0     // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 0, uint256("0x2a8e100939494904af825b488596ddd536b3a96226ad02e0f7ab7ae472b27a8e"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1428109151,
        1,
        1
    };

    static MapCheckpoints mapCheckpointsRegtest =
        boost::assign::map_list_of
        ( 0, uint256("0x2a8e100939494904af825b488596ddd536b3a96226ad02e0f7ab7ae472b27a8e"))
        ;
    static const CCheckpointData dataRegtest = {
        &mapCheckpointsRegtest,
        1428109151,
        1,
        1
    };

    const CCheckpointData &Checkpoints() {
        if (Params().NetworkID() == CChainParams::TESTNET)
            return dataTestnet;
        else if (Params().NetworkID() == CChainParams::MAIN)
            return data;
        else
            return dataRegtest;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!fEnabled)
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex, bool fSigchecks) {
        if (pindex==NULL)
            return 0.0;

        int64_t nNow = time(NULL);

        double fSigcheckVerificationFactor = fSigchecks ? SIGCHECK_VERIFICATION_FACTOR : 1.0;
        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is: 1.0 per TX before the last checkpoint, and fSigcheckVerificationFactor per TX after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!fEnabled)
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!fEnabled)
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
