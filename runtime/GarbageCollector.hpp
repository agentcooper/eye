// See `/docs/garbage-collector`

#pragma once

#include <map>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "debug.hpp"

namespace GarbageCollector {

constexpr size_t maximumBytes = 1024;

intptr_t *stackPointer;
intptr_t *stackStartPointer;

// only for AArch64
#define __READ_STACK_POINTER()                                                 \
  __asm__ volatile("mov %0, sp" : "=r"(stackPointer))

void run();

size_t totalBytesAllocated = 0;

struct Memory;

struct MemoryHeader {
  bool marked;
  size_t size;
};

std::map<Memory *, MemoryHeader> allocations;

struct Memory {
  MemoryHeader &getHeader() { return allocations.at(this); }
  virtual ~Memory(){};
};

void *allocate(const size_t bytes) {
  debug("There are total of ", totalBytesAllocated,
        " bytes allocated, now allocating "
        "additional ",
        bytes, " bytes...");

  if (totalBytesAllocated + bytes >= maximumBytes) {
    run();
  }

  void *memory = malloc(bytes);

  auto header = MemoryHeader{.marked = false, .size = bytes};
  allocations.insert(std::make_pair((Memory *)memory, header));
  totalBytesAllocated += bytes;

  // printf("[Runtime] Allocated %zu bytes at %p\n", bytes, memory);

  return memory;
}

std::vector<Memory *> getRoots() {
  std::vector<Memory *> result;

  auto sp = (uint8_t *)stackPointer;
  auto top = (uint8_t *)stackStartPointer;

  // printf("[Runtime] Scanning from %p to %p, total of %ld addresses\n",
  //        (void *)sp, (void *)top, top - sp);

  while (sp < top) {
    auto address = (Memory *)*(uintptr_t *)sp;
    if (allocations.contains(address)) {
      result.emplace_back(address);

      // printf("[Runtime] At address %p found pointer to %p\n", (void *)sp,
      //        (void *)address);
    }
    sp++;
  }

  return result;
}

std::vector<Memory *> getPointers(Memory *memory) {
  auto p = (uint8_t *)memory;
  auto end = (p + memory->getHeader().size);
  std::vector<Memory *> result;
  while (p < end) {
    auto address = (Memory *)*(uintptr_t *)p;
    if (allocations.contains(address)) {
      result.emplace_back(address);
    }
    p++;
  }
  return result;
}

void init() {
  debug("GC init");

  stackStartPointer = (intptr_t *)__builtin_frame_address(0);
}

void mark() {
  auto roots = getRoots();
  while (!roots.empty()) {
    auto root = roots.back();
    roots.pop_back();
    auto &header = root->getHeader();
    if (!header.marked) {
      // printf("[Runtime] Marking memory at %p\n", (void *)root);
      header.marked = true;
      for (const auto &pointer : getPointers(root)) {
        roots.push_back(pointer);
      }
    }
  }
}

void sweep() {
  unsigned totalSweeped = 0;

  auto it = allocations.begin();
  while (it != allocations.end()) {
    if (it->second.marked) {
      it->second.marked = false;
      ++it;
    } else {
      totalSweeped += 1;
      free(it->first);
      totalBytesAllocated -= it->second.size;
      it = allocations.erase(it);
    }
  }

  debug("Done sweeping, sweeped ", totalSweeped);
}

void printMemoryStatus() {
  debug("There are total of ", totalBytesAllocated, " bytes allocated");
}

void markAndSweep() {
  mark();
  sweep();
}

void run() {
  // push registers to the stack
  jmp_buf jb;
  setjmp(jb);

  // read stack address here to minimize the risk of scanning previous data when
  // the stack grows back
  __READ_STACK_POINTER();

  printMemoryStatus();
  markAndSweep();
  printMemoryStatus();
}

} // namespace GarbageCollector