// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "chain.h"
#include "chainparams.h"
#include "main.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <math.h>

unsigned int DarkGravityWave(const CBlockIndex* pindexLast, const CBlockHeader* pblock)
{
    /* current difficulty formula, kgs - DarkGravity v3, written by Evan Duffield - evan@dashpay.io */
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    uint256 PastDifficultyAverage;
    uint256 PastDifficultyAveragePrev;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return Params().ProofOfWorkLimit().GetCompact();
    }

    if (pindexLast->nHeight > Params().LAST_POW_BLOCK()) {
        uint256 bnTargetLimit = (~uint256(0) >> 24);
        int64_t nTargetSpacing = 150; // 150 second blocks
        int64_t nTargetTimespan = 60 * 40;

        int64_t nActualSpacing = 0;
        if (pindexLast->nHeight != 0)
            nActualSpacing = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();

        if (nActualSpacing < 0)
            nActualSpacing = 1;

        // ppcoin: target change every block
        // ppcoin: retarget with exponential moving toward target spacing
        uint256 bnNew;
        bnNew.SetCompact(pindexLast->nBits);

        int64_t nInterval = nTargetTimespan / nTargetSpacing;
        bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
        bnNew /= ((nInterval + 1) * nTargetSpacing);

        if (bnNew <= 0 || bnNew > bnTargetLimit)
            bnNew = bnTargetLimit;

        return bnNew.GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) {
            break;
        }
        CountBlocks++;

        if (CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) {
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
            } else {
                PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks) + (uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1);
            }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if (LastBlockTime > 0) {
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    uint256 bnNew(PastDifficultyAverage);

    int64_t _nTargetTimespan = CountBlocks * Params().TargetSpacing();

    if (nActualTimespan < _nTargetTimespan / 3)
        nActualTimespan = _nTargetTimespan / 3;
    if (nActualTimespan > _nTargetTimespan * 3)
        nActualTimespan = _nTargetTimespan * 3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > Params().ProofOfWorkLimit()) {
        bnNew = Params().ProofOfWorkLimit();
    }

    return bnNew.GetCompact();
}

unsigned int Terminal_Velocity_RateX(const CBlockIndex* pindexLast, const CBlockHeader* pblock)
{
       // Terminal-Velocity-RateX, v10-Beta-R7.1 (Pure PoS), written by Jonathan Dan Zaretsky - cryptocoderz@gmail.com
       const uint256 bnTerminalVelocity = Params().ProofOfWorkLimit();
       // Define values
       double VLF1 = 0;
       double VLF2 = 0;
       double VLF3 = 0;
       double VLF4 = 0;
       double VLF5 = 0;
       double VLFtmp = 0;
       double VRFsm1 = 1;
       double VRFdw1 = 0.75;
       double VRFdw2 = 0.5;
       double VRFup1 = 1.25;
       double VRFup2 = 1.5;
       double VRFup3 = 2;
       double TerminalAverage = 0;
       double TerminalFactor = 10000;
       int64_t VLrate1 = 0;
       int64_t VLrate2 = 0;
       int64_t VLrate3 = 0;
       int64_t VLrate4 = 0;
       int64_t VLrate5 = 0;
       int64_t VLRtemp = 0;
       int64_t DSrateNRM = 150; // 150 second blocks;
       int64_t DSrateMAX = DSrateNRM + 30;
       int64_t FRrateDWN = DSrateNRM - 30;
       int64_t FRrateFLR = DSrateNRM - 90;
       int64_t FRrateCLNG = DSrateNRM + 90;
       int64_t difficultyfactor = 0;
       int64_t AverageDivisor = 5;
       int64_t scanheight = 6;
       int64_t scanblocks = 1;
       int64_t scantime_1 = 0;
       int64_t scantime_2 = pindexLast->GetBlockTime();
       // Check for blocks to index | Allowing for diff reset during toggle
       if (pindexLast->nHeight < 9999999) {// TODO: Set fork block
           return bnTerminalVelocity.GetCompact(); // reset diff for transition
       }
       // Set prev blocks...
       const CBlockIndex* pindexPrev = pindexLast;
       // ...and deduce spacing
       while(scanblocks < scanheight)
       {
           scantime_1 = scantime_2;
           pindexPrev = pindexPrev->pprev;
           scantime_2 = pindexPrev->GetBlockTime();
           // Set standard values
           if(scanblocks > 0){
               if     (scanblocks < scanheight-4){ VLrate1 = (scantime_1 - scantime_2); VLRtemp = VLrate1; }
               else if(scanblocks < scanheight-3){ VLrate2 = (scantime_1 - scantime_2); VLRtemp = VLrate2; }
               else if(scanblocks < scanheight-2){ VLrate3 = (scantime_1 - scantime_2); VLRtemp = VLrate3; }
               else if(scanblocks < scanheight-1){ VLrate4 = (scantime_1 - scantime_2); VLRtemp = VLrate4; }
               else if(scanblocks < scanheight-0){ VLrate5 = (scantime_1 - scantime_2); VLRtemp = VLrate5; }
           }
           // Round factoring
           if(VLRtemp >= DSrateNRM){ VLFtmp = VRFsm1;
               if(VLRtemp > DSrateMAX){ VLFtmp = VRFdw1;
                   if(VLRtemp > FRrateCLNG){ VLFtmp = VRFdw2; }
               }
           }
           else if(VLRtemp < DSrateNRM){ VLFtmp = VRFup1;
               if(VLRtemp < FRrateDWN){ VLFtmp = VRFup2;
                   if(VLRtemp < FRrateFLR){ VLFtmp = VRFup3; }
               }
           }
           // Record factoring
           if      (scanblocks < scanheight-4) VLF1 = VLFtmp;
           else if (scanblocks < scanheight-3) VLF2 = VLFtmp;
           else if (scanblocks < scanheight-2) VLF3 = VLFtmp;
           else if (scanblocks < scanheight-1) VLF4 = VLFtmp;
           else if (scanblocks < scanheight-0) VLF5 = VLFtmp;
           // move up per scan round
           scanblocks ++;
       }
       // Final mathematics
       TerminalAverage = (VLF1 + VLF2 + VLF3 + VLF4 + VLF5) / AverageDivisor;
       // Log PoS prev block
       const CBlockIndex* BlockVelocityType = pindexLast;
       // Retarget
       uint256 bnOld;
       uint256 bnNew;
       TerminalFactor *= TerminalAverage;
       difficultyfactor = TerminalFactor;
       bnOld.SetCompact(BlockVelocityType->nBits);
       bnNew = bnOld / difficultyfactor;
       bnNew *= 10000;
       // Limit
       if (bnNew > bnTerminalVelocity)
         bnNew = bnTerminalVelocity;
       // Print for debugging
       if(fDebug) {
           LogPrintf("Terminal-Velocity 1st spacing: %u: \n",VLrate1);
           LogPrintf("Terminal-Velocity 2nd spacing: %u: \n",VLrate2);
           LogPrintf("Terminal-Velocity 3rd spacing: %u: \n",VLrate3);
           LogPrintf("Terminal-Velocity 4th spacing: %u: \n",VLrate4);
           LogPrintf("Terminal-Velocity 5th spacing: %u: \n",VLrate5);
           LogPrintf("Desired normal spacing: %u: \n",DSrateNRM);
           LogPrintf("Desired maximum spacing: %u: \n",DSrateMAX);
           LogPrintf("Terminal-Velocity 1st multiplier set to: %f: \n",VLF1);
           LogPrintf("Terminal-Velocity 2nd multiplier set to: %f: \n",VLF2);
           LogPrintf("Terminal-Velocity 3rd multiplier set to: %f: \n",VLF3);
           LogPrintf("Terminal-Velocity 4th multiplier set to: %f: \n",VLF4);
           LogPrintf("Terminal-Velocity 5th multiplier set to: %f: \n",VLF5);
           LogPrintf("Terminal-Velocity averaged a final multiplier of: %f: \n",TerminalAverage);
           LogPrintf("Prior Terminal-Velocity: %08x  %s\n", BlockVelocityType->nBits, bnOld.ToString());
           LogPrintf("New Terminal-Velocity:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());
       }
       return bnNew.GetCompact();
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock)
{
    // Default with VRX
    unsigned int retarget = DIFF_VRX;


    // Check for VRX activation
    if (pindexLast->nHeight < 9999999) {// TODO: Set fork block
        LogPrintf("DarkGravityWave retarget selected \n");
        retarget = DIFF_DGW;
    }
    // Check selection
    if (retarget != DIFF_VRX) {
        LogPrintf("GetNextTargetRequired() : Pre-VRX fork, using DGWv3 diff retarget \n");
        return DarkGravityWave(pindexLast, pblock);
    }

    // Retarget using Terminal-Velocity
    // debug info for testing
    LogPrintf("Terminal-Velocity retarget selected \n");
    LogPrintf("KingsCoin retargetted using: Terminal-Velocity difficulty curve \n");
    return Terminal_Velocity_RateX(pindexLast, pblock);

}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;

    if (Params().SkipProofOfWorkCheck())
        return true;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit())
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget)
       return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

uint256 GetBlockProof(const CBlockIndex& block)
{
    uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}
