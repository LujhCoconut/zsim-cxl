/** $lic$
 * Copyright (C) 2012-2015 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work.
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TIMING_CORE_H_
#define TIMING_CORE_H_

#include "core.h"
#include "core_recorder.h"
#include "event_recorder.h"
#include "memory_hierarchy.h"
#include "pad.h"

class FilterCache;

class TimingCore : public Core {
    private:
        FilterCache* l1i;
        FilterCache* l1d;

        uint64_t instrs;

        uint64_t curCycle; //phase 1 clock
        uint64_t phaseEndCycle; //phase 1 end clock

        CoreRecorder cRec;

    public:
        TimingCore(FilterCache* _l1i, FilterCache* _l1d, uint32_t domain, g_string& _name);
        void initStats(AggregateStat* parentStat);

        uint64_t getInstrs() const {return instrs;}
        uint64_t getPhaseCycles() const;
        uint64_t getCycles() const {return cRec.getUnhaltedCycles(curCycle);}

        void contextSwitch(int32_t gid);
        virtual void join();
        virtual void leave();

        InstrFuncPtrs GetFuncPtrs();

        //Contention simulation interface
        inline EventRecorder* getEventRecorder() {return cRec.getEventRecorder();}
        void cSimStart() override {curCycle = cRec.cSimStart(curCycle);}
        void cSimEnd() override {curCycle = cRec.cSimEnd(curCycle);}
        bool needsCSim() const override { return true; }

    private:
        inline void loadAndRecord(Address addr);
        inline void storeAndRecord(Address addr);
        inline void bblAndRecord(Address bblAddr, BblInfo* bblInstrs);
        inline void record(uint64_t startCycle);

        static void LoadAndRecordFunc(THREADID tid, ADDRINT addr);
        static void StoreAndRecordFunc(THREADID tid, ADDRINT addr);
        static void BblAndRecordFunc(THREADID tid, ADDRINT bblAddr, BblInfo* bblInfo);
        static void PredLoadAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred);
        static void PredStoreAndRecordFunc(THREADID tid, ADDRINT addr, BOOL pred);

        static void BranchFunc(THREADID, ADDRINT, BOOL, ADDRINT, ADDRINT) {}
} ATTR_LINE_ALIGNED;

#endif  // TIMING_CORE_H_
