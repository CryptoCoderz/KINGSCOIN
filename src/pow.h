// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;
class arith_uint256;

// Define difficulty retarget algorithms
enum DiffMode {
    DIFF_DEFAULT = 0, // Default to invalid 0
    DIFF_DGW = 1,     // Retarget using Dark Gravity Wave v3
    DIFF_VRX = 2,     // Retarget using Terminal-Velocity-RateX
};

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits);
uint256 GetBlockProof(const CBlockIndex& block);

#endif // BITCOIN_POW_H
