// Copyright (c) 2014-2016 The Dash developers
// Copyright (c) 2016-2018 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "masternode-budget.h"
#include "net.h"
#include "spork.h"
#include "sporkdb.h"


#define MAKE_SPORK_DEF(name, defaultValue) CSporkDef(name, defaultValue, #name)

std::vector<CSporkDef> sporkDefs = {
    MAKE_SPORK_DEF(SPORK_2_SWIFTTX,                         0),             // ON
    MAKE_SPORK_DEF(SPORK_3_SWIFTTX_BLOCK_FILTERING,         0),             // ON
    MAKE_SPORK_DEF(SPORK_5_MAX_VALUE,                       1000),          // 1000
    MAKE_SPORK_DEF(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT,  4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT,   4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_10_MASTERNODE_PAY_UPDATED_NODES,   0),             // OFF
    MAKE_SPORK_DEF(SPORK_13_ENABLE_SUPERBLOCKS,             4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_14_NEW_PROTOCOL_ENFORCEMENT,       4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2,     4070908800ULL), // OFF
    MAKE_SPORK_DEF(SPORK_16_ZEROCOIN_MAINTENANCE_MODE,      0),             // ON
    MAKE_SPORK_DEF(SPORK_17_TX_FILTER,                      0),             // OFF
};

CSporkManager sporkManager;
std::map<uint256, CSporkMessage> mapSporks;

std::map<CBitcoinAddress, int64_t> mapFilterAddress;
bool txFilterState = false;
int txFilterTarget = 0;

CSporkManager::CSporkManager()
{
    for (auto& sporkDef : sporkDefs) {
        sporkDefsById.emplace(sporkDef.sporkId, &sporkDef);
        sporkDefsByName.emplace(sporkDef.name, &sporkDef);
    }
}

void CSporkManager::Clear()
{
    strMasterPrivKey = "";
    mapSporksActive.clear();
}

// PIVX: on startup load spork values from previous session if they exist in the sporkDB
void CSporkManager::LoadSporksFromDB()
{
    for (const auto& sporkDef : sporkDefs) {
        // attempt to read spork from sporkDB
        CSporkMessage spork;
        if (!pSporkDB->ReadSpork(sporkDef.sporkId, spork)) {
            LogPrintf("%s : no previous value for %s found in database\n", __func__, sporkDef.name);
            continue;
        }

        // add spork to memory
        mapSporks[spork.GetHash()] = spork;
        mapSporksActive[spork.nSporkID] = spork;
        std::time_t result = spork.nValue;
        // If SPORK Value is greater than 1,000,000 assume it's actually a Date and then convert to a more readable format
        if (spork.nValue > 1000000) {
            LogPrintf("%s : loaded spork %s with value %d : %s", __func__,
                      sporkManager.GetSporkNameByID(spork.nSporkID), spork.nValue,
                      std::ctime(&result));
        } else {
            LogPrintf("%s : loaded spork %s with value %d\n", __func__,
                      sporkManager.GetSporkNameByID(spork.nSporkID), spork.nValue);
        }
    }
}

void CSporkManager::ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (fLiteMode || chainActive.Tip() == nullptr) return; // disable all obfuscation/masternode related functionality

    if (strCommand == "spork") {

        CSporkMessage spork;
        vRecv >> spork;

        // Ignore spork messages about unknown/deleted sporks
        std::string strSpork = sporkManager.GetSporkNameByID(spork.nSporkID);
        if (strSpork == "Unknown") return;

        // Do not accept sporks signed way too far into the future
        if (spork.nTimeSigned > GetAdjustedTime() + 2 * 60 * 60) {
            LOCK(cs_main);
            LogPrintf("%s : ERROR: too far into the future\n", __func__);
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        uint256 hash = spork.GetHash();
        {
            LOCK(cs);
            if (mapSporksActive.count(spork.nSporkID)) {
                // spork is active
                if (mapSporksActive[spork.nSporkID].nTimeSigned >= spork.nTimeSigned) {
                    // spork in memory has been signed more recently
                    if (fDebug) LogPrintf("%s : seen %s block %d \n", __func__, hash.ToString(), chainActive.Tip()->nHeight);
                    return;
                } else {
                    // update active spork
                    if (fDebug) LogPrintf("%s : got updated spork %s block %d \n", __func__, hash.ToString(), chainActive.Tip()->nHeight);
                }
            } else {
                // spork is not active
                if (fDebug) LogPrintf("%s : got new spork %s block %d \n", __func__, hash.ToString(), chainActive.Tip()->nHeight);
            }
        }

        LogPrintf("%s : new %s ID %d Time %d bestHeight %d\n", __func__, hash.ToString(), spork.nSporkID, spork.nValue, chainActive.Tip()->nHeight);

        bool fRequireNew = spork.nTimeSigned >= Params().NewSporkStart();
        if (!spork.CheckSignature(fRequireNew)) {
            LOCK(cs_main);
            LogPrintf("%s : Invalid Signature\n", __func__);
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        {
            LOCK(cs);
            mapSporks[hash] = spork;
            mapSporksActive[spork.nSporkID] = spork;
        }
        spork.Relay();

        // PIVX: add to spork database.
        pSporkDB->WriteSpork(spork.nSporkID, spork);
        ExecuteSpork(spork.nSporkID, spork.nValue);
    }
    if (strCommand == "getsporks") {
        LOCK(cs);
        std::map<SporkId, CSporkMessage>::iterator it = mapSporksActive.begin();

        while (it != mapSporksActive.end()) {
            pfrom->PushMessage("spork", it->second);
            it++;
        }
    }
}

void CSporkManager::ExecuteSpork(SporkId nSporkID, int64_t nValue)
{
    if (nSporkID == SPORK_17_TX_FILTER) {
        LogPrintf("ExecuteSpork -- Initialize TX filter list\n");
        BuildTxFilter();
    }
}

void InitTxFilter()
{
    mapFilterAddress.clear();
    if (Params().NetworkID() == CBaseChainParams::MAIN) {
        //mapFilterAddress.emplace( CBitcoinAddress("address"), 1573206465 );
    }
}

void BuildTxFilter()
{
    LOCK(cs_main);

    InitTxFilter();
    CBitcoinAddress Address;
    CTxDestination Dest;

    CBlock referenceBlock;
    uint64_t sporkBlockValue = (sporkManager.GetSporkValue(SPORK_17_TX_FILTER) >> 32) & 0xffffffff; // 32-bit block number

    txFilterTarget = sporkBlockValue; // set filter targed on spork recived
    if (txFilterTarget == 0) {
        // no target block, return
        txFilterState = true;
        return;
    }

    CBlockIndex *referenceIndex = chainActive[sporkBlockValue];
    if (referenceIndex != NULL) {
        assert(ReadBlockFromDisk(referenceBlock, referenceIndex));
        int sporkMask = sporkManager.GetSporkValue(SPORK_17_TX_FILTER) & 0xffffffff; // 32-bit tx mask
        int nAddressCount = 0;

        // Find the addresses that we want filtered
        for (unsigned int i = 0; i < referenceBlock.vtx.size(); i++) {
            // The mask can support up to 32 transaction indexes (as it is 32-bit)
            if (((sporkMask >> i) & 0x1) != 0) {
                for (unsigned int j = 0; j < referenceBlock.vtx[i].vout.size(); j++) {
                    if (referenceBlock.vtx[i].vout[j].nValue > 0) {
                        ExtractDestination(referenceBlock.vtx[i].vout[j].scriptPubKey, Dest);
                        Address.Set(Dest);
                        auto it  = mapFilterAddress.emplace(Address, referenceBlock.GetBlockTime());

                        if (/*fDebug &&*/ it.second)
                            LogPrintf("BuildTxFilter(): Add Tx filter address %d in reference block %ld, %s\n",
                                          ++nAddressCount, sporkBlockValue, Address.ToString());
                    }
                }
            }
        }
        // filter initialization completed
        txFilterState = true;
    }
}

bool CSporkManager::UpdateSpork(SporkId nSporkID, int64_t nValue)
{

    CSporkMessage spork = CSporkMessage(nSporkID, nValue, GetTime());

    if(spork.Sign(strMasterPrivKey)){
        spork.Relay();
        LOCK(cs);
        mapSporks[spork.GetHash()] = spork;
        mapSporksActive[nSporkID] = spork;
        ExecuteSpork(spork.nSporkID, spork.nValue);
        return true;
    }

    return false;
}

// grab the spork value, and see if it's off
bool CSporkManager::IsSporkActive(SporkId nSporkID)
{
    return GetSporkValue(nSporkID) < GetAdjustedTime();
}

// grab the value of the spork on the network, or the default
int64_t CSporkManager::GetSporkValue(SporkId nSporkID)
{
    LOCK(cs);

    if (mapSporksActive.count(nSporkID)) {
        return mapSporksActive[nSporkID].nValue;

    } else {
        auto it = sporkDefsById.find(nSporkID);
        if (it != sporkDefsById.end()) {
            return it->second->defaultValue;
        } else {
            LogPrintf("%s : Unknown Spork %d\n", __func__, nSporkID);
        }
    }

    return -1;
}

SporkId CSporkManager::GetSporkIDByName(std::string strName)
{
    auto it = sporkDefsByName.find(strName);
    if (it == sporkDefsByName.end()) {
        LogPrintf("%s : Unknown Spork name '%s'\n", __func__, strName);
        return SPORK_INVALID;
    }
    return it->second->sporkId;
}

std::string CSporkManager::GetSporkNameByID(SporkId nSporkID)
{
    auto it = sporkDefsById.find(nSporkID);
    if (it == sporkDefsById.end()) {
        LogPrint("%s : Unknown Spork ID %d\n", __func__, nSporkID);
        return "Unknown";
    }
    return it->second->name;
}

bool CSporkManager::SetPrivKey(std::string strPrivKey)
{
    CSporkMessage spork;

    spork.Sign(strPrivKey);

    const bool fRequireNew = GetTime() >= Params().NewSporkStart();
    if (spork.CheckSignature(fRequireNew)) {
        LOCK(cs);
        // Test signing successful, proceed
        LogPrintf("%s : Successfully initialized as spork signer\n", __func__);
        strMasterPrivKey = strPrivKey;
        return true;
    }

    return false;
}

std::string CSporkManager::ToString() const
{
    LOCK(cs);
    return strprintf("Sporks: %llu", mapSporksActive.size());
}

bool CSporkMessage::Sign(std::string strSignKey)
{
    std::string strMessage = std::to_string(nSporkID) + std::to_string(nValue) + std::to_string(nTimeSigned);

    CKey key;
    CPubKey pubkey;
    std::string errorMessage = "";

    if (!obfuScationSigner.SetKey(strSignKey, errorMessage, key, pubkey)) {
        return error("%s : SetKey error: '%s'\n", __func__, errorMessage);
    }

    if (!obfuScationSigner.SignMessage(strMessage, errorMessage, vchSig, key)) {
        return error("%s : Sign message failed", __func__);
    }

    if (!obfuScationSigner.VerifyMessage(pubkey, vchSig, strMessage, errorMessage)) {
        return error("%s : Verify message failed", __func__);
    }

    return true;
}

bool CSporkMessage::CheckSignature(bool fRequireNew)
{
    //note: need to investigate why this is failing
    std::string strMessage = std::to_string(nSporkID) + std::to_string(nValue) + std::to_string(nTimeSigned);
    CPubKey pubkeynew(ParseHex(Params().SporkPubKey()));
    std::string errorMessage = "";

    bool fValidWithNewKey = obfuScationSigner.VerifyMessage(pubkeynew, vchSig, strMessage, errorMessage);

    if (fRequireNew && !fValidWithNewKey)
        return false;

    return fValidWithNewKey;
}

void CSporkMessage::Relay()
{
    CInv inv(MSG_SPORK, GetHash());
    RelayInv(inv);
}
