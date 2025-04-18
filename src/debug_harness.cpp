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

#include "debug_harness.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include "log.h"
#include "str.h"

//For funky macro stuff
#define QUOTED_(x) #x
#define QUOTED(x) QUOTED_(x)

/* This file is pretty much self-contained, and has minimal external dependencies.
 * Please keep it this way, and ESPECIALLY don't include Pin headers since there
 * seem to be conflicts between those and some system headers.
 */

// Note: the following function is used when sim.debugPortId is not 0. A separate debugger client script should be listening.
int launchGDBDebugger(int targetPid, LibInfo* libzsimAddrs, int debugPortId) {
    std::string targetPidStr = Str(targetPid);
    char symbolCmdStr[2048];
    snprintf(symbolCmdStr, sizeof(symbolCmdStr), "\'add-symbol-file %s %p -s .data %p -s .bss %p\'", QUOTED(ZSIM_PATH), libzsimAddrs->textAddr, libzsimAddrs->dataAddr, libzsimAddrs->bssAddr);

    const char* const args[] = {"gdb", "-p", targetPidStr.c_str(),
        "-ex", "\'set confirm off\'", //we know what we're doing in the following 2 commands
        "-ex", symbolCmdStr,
        "-ex", "\'handle SIGTRAP nostop noprint\'", // For some reason we receive a lot of spurious sigtraps
        "-ex", "\'set confirm on\'", //reenable confirmations
        // "-ex", "c", //trap at start
        nullptr};
    int id = 0;
    std::string complete_args = "";
    while (args[id]!=nullptr) {
        complete_args += args[id++];
        complete_args += " ";
    }
    char port_str[8];
    sprintf(port_str, "%d", debugPortId);
    complete_args = "echo \"" + complete_args + "\" | nc localhost " + port_str; // pass pid to debug_cli.sh
    // printf("DEBUG args %s\n", complete_args.c_str());
    FILE* fp = popen(complete_args.c_str(), "r");
    if (fp) pclose(fp);
    // zsim continues to run even no client is established. A friendly prompt can be added here.
    return 0;
}

// This is the tradition Xterm debugger. A xterm shell (GUI window) is popped out if you have installed X Window System.
int launchXtermDebugger(int targetPid, LibInfo* libzsimAddrs) {
    int childPid = fork();
    if (childPid == 0) {
        std::string targetPidStr = Str(targetPid);
        char symbolCmdStr[2048];
        snprintf(symbolCmdStr, sizeof(symbolCmdStr), "add-symbol-file %s %p -s .data %p -s .bss %p", QUOTED(ZSIM_PATH), libzsimAddrs->textAddr, libzsimAddrs->dataAddr, libzsimAddrs->bssAddr);

        const char* const args[] = {"xterm", "-e", "gdb", "-p", targetPidStr.c_str(),
            "-ex", "set confirm off", //we know what we're doing in the following 2 commands
            "-ex", symbolCmdStr,
            "-ex", "handle SIGTRAP nostop noprint", // For some reason we receive a lot of spurious sigtraps
            "-ex", "set confirm on", //reenable confirmations
            "-ex", "c", //start running
            nullptr};
        execvp(args[0], (char* const*)args);
        panic("shouldn't reach this...");
    } else {
        return childPid;
    }
}
