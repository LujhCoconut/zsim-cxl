#!/usr/bin/python

# Copyright (C) 2013-2015 by Massachusetts Institute of Technology
#
# This file is part of zsim.
#
# zsim is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, version 2.
#
# If you use this software in your research, we request that you reference
# the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
# Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
# source of the simulator in any publications that use this software, and that
# you send us a citation of your work.
#
# zsim is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <http://www.gnu.org/licenses/>.

import os, string, sys

class XTemplate(string.Template):
    delimiter = "$"
    escaped = "$$"

def cmd(c):
    f = os.popen(c)
    r = f.read()
    f.close()
    return r.rstrip() # rm last newline

def getMask(start, end):
    cur = 0
    l = []
    for i in range(256):
        j = i % 32
        if i >= start and i <= end: cur |= 1 << j
        if (i + 1) % 32 == 0:
            l.append(cur)
            cur = 0
    l.reverse()
    return ",".join("%08x" % n for n in l)

from optparse import OptionParser

# Read in options & args
parser = OptionParser(usage="%prog [options] [resultsDirSuffix]")
parser.add_option("-n", type="int", dest="ncpus", default=1, help="Number of simulated cores")
parser.add_option("-d", type="string", dest="dir", default="./patchRoot", help="Destination directory")
parser.add_option("-f", dest="force", action="store_true", default=False, help="Force, bypass existence checks")
parser.add_option("--nodes", type="int", dest="nnodes", default=1, help="Number of simulated NUMA nodes")


for option in parser.option_list:
    if option.default != ("NO", "DEFAULT"):
        option.help += (" " if option.help else "") + "[default: %default]"
(options, args) = parser.parse_args()

ncpus = options.ncpus
root = options.dir
progDir = os.path.dirname(os.path.abspath(__file__)) + "/"
nnodes = options.nnodes

print "Will produce a tree for %d CPUs/cores with %d NUMA nodes in %s" % (ncpus, nnodes, root)

if ncpus < 1:
    print "ERROR: Need >= 1 cpus!"
    sys.exit(1)

ncpusPerNode = ncpus / nnodes
if ncpus % nnodes != 0:
    print "ERROR: CPUs (%d) must be evenly distributed among %d NUMA nodes!" % (ncpus, nnodes)
    sys.exit(1)

if os.path.exists(root) and not options.force:
    print "ERROR: Dir already exists, aborting"
    sys.exit(1)

if len(args):
    print "ERROR: No positional arguments taken, aborting"
    sys.exit(1)

cmd("mkdir -p " + root)
if not os.path.exists(root):
    print "ERROR: Could not create %s, aborting" % root
    sys.exit(1)

## /proc

# cpuinfo
cpuinfoTemplate = XTemplate(open(progDir + "cpuinfo.template", "r").read())
cmd("mkdir -p %s/proc" % root)
f = open(root + "/proc/cpuinfo", "w")
for cpu in range(ncpus):
    print >>f, cpuinfoTemplate.substitute({"CPU" : str(cpu), "NCPUS" : ncpus}),
f.close()

# stat
cpuAct = [int(x) for x in "665084 119979939 9019834 399242499 472611 20 159543 0 0 0".split(" ")]
totalAct = [x*ncpus for x in cpuAct]
cpuStat = "cpu  " + " ".join([str(x) for x in totalAct])
for cpu in range(ncpus):
    cpuStat += ("\ncpu%d " % cpu) + " ".join([str(x) for x in cpuAct])
statTemplate = XTemplate(open(progDir + "stat.template", "r").read())
f = open(root + "/proc/stat", "w")
print >>f, statTemplate.substitute({"CPUSTAT" : cpuStat}),
f.close()

# self/status
cmd("mkdir -p %s/proc/self" % root)
f = open(root + "/proc/self/status", "w")
# FIXME: only for CPU/memory list
print >>f, "..."
print >>f, "Cpus_allowed:\t%s" % getMask(0, ncpus-1)
print >>f, "Cpus_allowed_list:\t%s" % ("0-" + str(ncpus-1) if ncpus > 1 else "0")
print >>f, "Mems_allowed:\t%s" % getMask(0, nnodes-1)
print >>f, "Mems_allowed_list:\t%s" % ("0-" + str(nnodes-1) if nnodes > 1 else "0")
print >>f, "..."
f.close()

## /sys

# cpus
cpuDir = root + "/sys/devices/system/cpu/"
cmd("mkdir -p " + cpuDir)
cpuList = "0-" + str(ncpus-1) if ncpus > 1 else "0"
for f in ["online", "possible", "present"]:
    cmd("echo %s > %s" % (cpuList, cpuDir + f))
cmd("echo > " + cpuDir + "offline")
cmd("echo 0 > " + cpuDir + "sched_mc_power_savings")
if ncpus > 255:
    print "WARN: These many cpus have not been tested, x2APIC systems may be different..."
maxCpus = max(ncpus, 255)
cmd("echo %d > %s" % (maxCpus, cpuDir + "kernel_max"))
for cpu in range(ncpus):
    node = cpu / ncpusPerNode
    coreSiblingsMask = getMask(node*ncpusPerNode, (node+1)*ncpusPerNode-1)
    cpuList = (str(node*ncpusPerNode) + "-" + str((node+1)*ncpusPerNode-1)) if ncpusPerNode > 1 else str(cpu)
    d = cpuDir + "cpu" + str(cpu) + "/"
    td = d + "topology/"
    cmd("mkdir -p " + td)
    cmd("echo %d > %s" % (cpu, td + "core_id"))
    cmd("echo %s > %s" % (cpuList, td + "core_siblings_list"))
    cmd("echo %d > %s" % (cpu, td + "thread_siblings_list"))
    cmd("echo %d > %s" % (node, td + "physical_package_id"))
    cmd("echo %s > %s" % (coreSiblingsMask, td + "core_siblings"))
    cmd("echo %s > %s" % (getMask(cpu, cpu), td + "thread_siblings"))
    cmd("echo 1 > " + d + "online")

# nodes
nodeDir = root + "/sys/devices/system/node/"
cmd("mkdir -p " + nodeDir)
nodeList = "0-" + str(nnodes-1)
for f in ["has_normal_memory", "online", "possible"]:
    cmd("echo %s > " % (nodeList if nnodes > 1 else "0") + nodeDir + f)
cmd("echo %s > " % (nodeList if nnodes > 1 else "")+ nodeDir + "has_cpu")

meminfoTemplate = XTemplate(open(progDir + "/nodeFiles/meminfo.template", "r").read())
for node in range(nnodes):
    nDir = nodeDir + "node%d/" % (node)
    cmd("mkdir -p " + nDir)
    for cpu in range(node*ncpusPerNode, (node+1)*ncpusPerNode):
        cmd("ln -s " + cpuDir + "cpu" + str(cpu) + " " + nDir)

    for f in ["numastat", "scan_unevictable_pages", "vmstat"]:
        cmd("cp -r %s/nodeFiles/%s %s" % (progDir, f, nDir))

    cpuMask = getMask(node*ncpusPerNode, (node+1)*ncpusPerNode-1)
    cpuList = (str(node*ncpusPerNode) + "-" + str((node+1)*ncpusPerNode-1)) if ncpusPerNode > 1 else str(cpu)
    cmd("echo %s > %s" % (cpuMask, nDir + "cpumap"))
    cmd("echo %s > %s" % (cpuList, nDir + "cpulist"))

    f = open(nDir + "/meminfo", "w")
    print >>f, meminfoTemplate.substitute({"NODE" : str(node)}),
    f.close()

    f = open(nDir + "/distance", "w")
    for n2 in range(nnodes):
        d = "10" if n2 == node else "20"
        print >>f, d + " ",
    print >>f, ""
    f.close()

# misc
cmd("mkdir -p " + root + "/sys/bus/pci/devices")

# make read-only
if not options.force:
    cmd("chmod a-w -R " + root)


