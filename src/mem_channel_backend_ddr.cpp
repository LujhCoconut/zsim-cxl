#include "mem_channel_backend_ddr.h"
#include <cstring>          // for strcmp
#include "bithacks.h"
#include "config.h"         // for Tokenize

// TODO: implement
// refresh: RFC, REFI, RPab
// adaptive close/open
// power-down: XP
// power model: IDD

MemChannelBackendDDR::MemChannelBackendDDR(const g_string& _name,
        uint32_t ranksPerChannel, uint32_t banksPerRank, const char* _pagePolicy,
        uint32_t pageSizeBytes, uint32_t burstCount, uint32_t deviceIOBits, uint32_t channelWidthBits,
        uint32_t memFreqMHz, const Timing& _t,
        const char* addrMapping, uint32_t _queueDepth)
    : name(_name), rankCount(ranksPerChannel), bankCount(banksPerRank),
      pageSize(pageSizeBytes), burstSize(burstCount*deviceIOBits),
      devicesPerRank(channelWidthBits/deviceIOBits), freqKHz(memFreqMHz*1000), t(_t),
      queueDepth(_queueDepth) {

    info("%s: %u ranks x %u banks.", name.c_str(), rankCount, bankCount);
    info("%s: page size %u bytes, %u devices per rank, burst %u bits from each device.",
            name.c_str(), pageSize, devicesPerRank, burstSize);
    info("%s: tBL = %u, tCAS = %u, tCCD = %u, tCWD = %u, tRAS = %u, tRCD = %u",
            name.c_str(), t.BL, t.CAS, t.CCD, t.CWD, t.RAS, t.RCD);
    info("%s: tRP = %u, tRRD = %u, tRTP = %u, tWR = %u, tWTR = %u",
            name.c_str(), t.RP, t.RRD, t.RTP, t.WR, t.WTR);
    info("%s: tRFC = %u, tREFI = %u, tRPab = %u, tFAW = %u, tRTRS = %u, tCMD = %u, tXP = %u",
            name.c_str(), t.RFC, t.REFI, t.RPab, t.FAW, t.RTRS, t.CMD, t.XP);

    assert_msg(channelWidthBits % deviceIOBits == 0,
            "Channel width (%u given) must be multiple of device IO width (%u given).",
            channelWidthBits, deviceIOBits);

    // Page policy.
    if (strcmp(_pagePolicy, "open") == 0) {
        pagePolicy = DDRPagePolicy::Open;
    } else if (strcmp(_pagePolicy, "close") == 0) {
        pagePolicy = DDRPagePolicy::Close;
    } else {
        panic("Unrecognized page policy %s", _pagePolicy);
    }

    // Banks.
    banks.clear();
    for (uint32_t r = 0; r < rankCount; r++) {
        auto aw = new DDRActWindow(4);
        for (uint32_t b = 0; b < bankCount; b++) {
            banks.emplace_back(aw);
        }
    }
    assert(banks.size() == rankCount * bankCount);

    // Address mapping.

    assert_msg(isPow2(rankCount), "Only support power-of-2 ranks per channel, %u given.", rankCount);
    assert_msg(isPow2(bankCount), "Only support power-of-2 banks per rank, %u given.", bankCount);
    // One column has the size of deviceIO. LSB bits of column address are for burst. Here we only account for the high
    // bits of column which are in line address, since the low bits are hidden in lineSize.
    uint32_t colHCount = pageSize*8 / burstSize;
    assert_msg(isPow2(colHCount), "Only support power-of-2 column bursts per row, %u given "
            "(page %u Bytes, device IO %u bits, %u bursts).", colHCount, pageSize, deviceIOBits, burstCount);

    uint32_t rankBitCount = ilog2(rankCount);
    uint32_t bankBitCount = ilog2(bankCount);
    uint32_t colHBitCount = ilog2(colHCount);

    std::vector<std::string> tokens;
    Tokenize(addrMapping, tokens, ":");
    if (tokens.size() != 3 && !(tokens.size() == 4 && tokens[0] == "row")) {
        panic("Wrong address mapping %s: row default at MSB, must contain bank, rank, and col.", addrMapping);
    }

    rankMask = bankMask = colHMask = 0;
    uint32_t startBit = 0;
    auto computeShiftMask = [&startBit, &addrMapping](const std::string& t, const uint32_t bitCount,
            uint32_t& shift, uint32_t& mask) {
        if (mask) panic("Repeated field %s in address mapping %s.", t.c_str(), addrMapping);
        shift = startBit;
        mask = (1 << bitCount) - 1;
        startBit += bitCount;
    };

    // Reverse order, from LSB to MSB.
    for (auto it = tokens.crbegin(); it != tokens.crend(); it++) {
        auto t = *it;
        if (t == "row") {}
        else if (t == "rank") computeShiftMask(t, rankBitCount, rankShift, rankMask);
        else if (t == "bank") computeShiftMask(t, bankBitCount, bankShift, bankMask);
        else if (t == "col")  computeShiftMask(t, colHBitCount, colHShift, colHMask);
        else panic("Invalid field %s in address mapping %s.", t.c_str(), addrMapping);
    }
    rowShift = startBit;
    rowMask = -1u;

    info("%s: Address mapping %s row %d:%u rank %u:%u bank %u:%u col %u:%u",
            name.c_str(), addrMapping, 63, rowShift,
            ilog2(rankMask << rankShift), rankMask ? rankShift : 0,
            ilog2(bankMask << bankShift), bankShift ? bankShift : 0,
            ilog2(colHMask << colHShift), colHShift ? colHShift : 0);

    // Schedule and issue.

    reqQueueRd.init(queueDepth);
    reqQueueWr.init(queueDepth);

    prioLists.resize(rankCount * bankCount);

    issueWrite = false;
    minBurstCycle = 0;
    lastIsWrite = false;
}

void MemChannelBackendDDR::initStats(AggregateStat* parentStat) {
    // TODO: implement
}

uint64_t MemChannelBackendDDR::enqueue(const Address& addr, const bool isWrite,
        uint64_t startCycle, uint64_t memCycle, MemChannelAccEvent* respEv) {
    // Allocate request.
    auto req = reqQueue(isWrite).alloc();
    req->addr = addr;
    req->isWrite = isWrite;
    req->startCycle = startCycle;
    req->schedCycle = memCycle;
    req->ev = respEv;

    req->loc = mapAddress(addr);

    // Assign priority.
    assignPriority(req);

    if (req->hasHighestPriority()) {
        return requestHandler(req);
    }
    else return -1uL;
}

bool MemChannelBackendDDR::dequeue(uint64_t memCycle, MemChannelAccReq** req, uint64_t* minTickCycle) {
    // Update read/write issue mode.
    issueWrite = reqQueueRd.empty() || reqQueueWr.size() > queueDepth * 3/4 ||
        (issueWrite && reqQueueWr.size() > queueDepth * 1/4);

    // Scan the requests with the highest priority in each list in chronological order.
    // Take the first request (the oldest) that is ready to be issued.
    DDRAccReq* r = nullptr;
    uint64_t mtc = -1uL;
    auto ir = reqQueue(issueWrite).begin();
    for (; ir != reqQueue(issueWrite).end(); ir.inc()) {
        if (!(*ir)->hasHighestPriority())
            continue;

        uint64_t c = requestHandler(*ir);
        if (c <= memCycle) {
            r = *ir;
            break;
        }
        mtc = std::min(mtc, c);
    }

    // If no request is ready, set min tick cycle.
    if (!r) {
        *minTickCycle = mtc;
        return false;
    }

    // Remove priority assignment.
    cancelPriority(r);

    *req = new DDRAccReq(*r);
    reqQueue(issueWrite).remove(ir);

    return true;
}

uint64_t MemChannelBackendDDR::process(const MemChannelAccReq* req) {
    const DDRAccReq* ddrReq = dynamic_cast<const DDRAccReq*>(req);
    assert(ddrReq);

    uint64_t burstCycle = requestHandler(ddrReq, true);
    uint64_t respCycle = burstCycle + t.BL;

    lastIsWrite = req->isWrite;
    lastRankIdx = ddrReq->loc.rank;
    minBurstCycle = respCycle;
    return respCycle;
}

bool MemChannelBackendDDR::queueOverflow(const bool isWrite) const {
    return reqQueue(isWrite).full();
}


uint64_t MemChannelBackendDDR::requestHandler(const DDRAccReq* req, bool update) {
    const auto& loc = req->loc;
    const auto& isWrite = req->isWrite;
    const auto& schedCycle = req->schedCycle;
    auto& bank = banks[loc.rank * bankCount + loc.bank];

    // Bank PRE.
    uint64_t preCycle = 0;
    if (!bank.open) {
        // Bank is closed.
        preCycle = bank.minPRECycle;
    } else if (loc.row != bank.row) {
        // A conflict row is open.
        preCycle = std::max(bank.minPRECycle, schedCycle);
        if (update) {
            bank.recordPRE(preCycle);
        }
    }

    // Bank ACT.
    uint64_t actCycle = 0;
    if (!bank.open || loc.row != bank.row) {
        // Need to open the row.
        actCycle = calcACTCycle(bank, schedCycle, preCycle);
        if (update) {
            bank.recordACT(actCycle, loc.row);
        }
    }
    if (update) {
        assert(bank.open && loc.row == bank.row);
    }

    // RD/WR.
    uint64_t rwCycle = calcRWCycle(bank, schedCycle, actCycle, isWrite, loc.rank);
    if (update) {
        bank.recordRW(rwCycle);
    }

    // Burst data transfer.
    uint64_t burstCycle = calcBurstCycle(bank, rwCycle, isWrite);
    assert(burstCycle >= minBurstCycle);

    // (future) next PRE.
    if (update) {
        // TODO: adaptive close
        uint64_t preCycle = updatePRECycle(bank, rwCycle, isWrite);
        if (pagePolicy == DDRPagePolicy::Close) {
            bank.recordPRE(preCycle);
        }
    }

    return burstCycle;
}

void MemChannelBackendDDR::assignPriority(DDRAccReq* req) {
    // TODO: implement
}

void MemChannelBackendDDR::cancelPriority(DDRAccReq* req) {
    // TODO: implement
}

uint64_t MemChannelBackendDDR::calcACTCycle(const Bank& bank, uint64_t schedCycle, uint64_t preCycle) const {
    // Constraints: tRP, tRRD, tFAW, tCMD.
    return maxN<uint64_t>(
            schedCycle,
            preCycle + t.RP,
            bank.lastACTCycle + t.RRD,
            bank.actWindow->minACTCycle() + t.FAW,
            preCycle + t.CMD);
}

uint64_t MemChannelBackendDDR::calcRWCycle(const Bank& bank, uint64_t schedCycle, uint64_t actCycle,
        bool isWrite, uint32_t rankIdx) const {
    // Constraints: tRCD, tWTR, tCCD, bus contention, tRTRS, tCMD.
    int64_t dataOnBus = minBurstCycle;
    if (rankIdx != lastRankIdx) {
        // Switch rank.
        dataOnBus += t.RTRS;
    }
    return maxN<uint64_t>(
            schedCycle,
            actCycle + t.RCD,
            (lastIsWrite && !isWrite) ? minBurstCycle + t.WTR : 0,
            bank.lastRWCycle + t.CCD,
            std::max<int64_t>(0, dataOnBus - (isWrite ? std::max(t.CWD, t.CAS) : t.CAS)), // avoid underflow
            actCycle + t.CMD);
}

uint64_t MemChannelBackendDDR::calcBurstCycle(const Bank& bank, uint64_t rwCycle, bool isWrite) const {
    // Constraints: tCAS, tCWD.
    return rwCycle + (isWrite ? std::max(t.CWD, t.CAS) : t.CAS);
}

uint64_t MemChannelBackendDDR::updatePRECycle(Bank& bank, uint64_t rwCycle, bool isWrite) {
    assert(bank.open);
    // Constraints: tRAS, tWR, tRTP, tCMD.
    bank.minPRECycle = maxN<uint64_t>(
            bank.minPRECycle,
            bank.lastACTCycle + t.RAS,
            isWrite ? minBurstCycle + t.WR : rwCycle + t.RTP,
            rwCycle + t.CMD);
    return bank.minPRECycle;
}

