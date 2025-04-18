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

#ifndef G_UNORDERED_SET_H_
#define G_UNORDERED_SET_H_

#include <functional>
#include <unordered_set>
#include "g_std/stl_galloc.h"

// PinCRT is pre-C++11 and has unordered_set in std::tr1.
#ifdef PIN_CRT
namespace std {
    namespace tr1 {};  // in case not defined
    using namespace tr1;
};
#endif  // PIN_CRT

template <typename K> class g_unordered_set : public std::unordered_set<K, std::hash<K>, std::equal_to<K>, StlGlobAlloc<K> > {};

#endif  // G_UNORDERED_SET_H_
