// Copyright 2023 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Platform-specific code for z/OS goes here. For the POSIX-compatible
// parts, the implementation is in platform-posix.cc.

// TODO(gabylb): zos - most functions here will be removed once mmap is fully
// implemented on z/OS, after which those in platform-posix.cc will be used.
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <mutex>
#include <unordered_map>

#include "src/base/platform/platform-posix.h"
#include "src/base/platform/platform.h"
#include "src/common/globals.h"

typedef unsigned long value_type;
typedef unsigned long key_type;
typedef std::unordered_map<key_type, value_type>::const_iterator cursor_t;

namespace {

bool gLogMemUsage = false;

[[clang::no_destroy]]
std::unordered_map<key_type, value_type> zalloc_map;
[[clang::no_destroy]]
std::mutex zalloc_map_mutex;

[[clang::no_destroy]]
std::unordered_map<key_type, value_type> mmap_map;
[[clang::no_destroy]]
std::mutex mmap_mutex;

__attribute__((constructor)) void init() {
  // Initialize Node.js environment variables in zoslib:
  zoslib_config_t config;
  init_zoslib_config(&config);
  init_zoslib(config);

  gLogMemUsage = __doLogMemoryUsage();
}

int OSFree(void* address, const size_t size) {
  // Helper used by OS::Free() and OS::Release().
  cursor_t c;
  {
    std::lock_guard<std::mutex> lock(zalloc_map_mutex);
    c = zalloc_map.find((key_type)address);
    if (c != zalloc_map.end()) {
      int result = __zfree((void*)c->second, size);
      zalloc_map.erase(c);
      return result;
    }
  }
  std::lock_guard<std::mutex> lock(mmap_mutex);
  c = mmap_map.find((key_type)address);
  if (c != mmap_map.end()) {
    int result = munmap(address, ((size_t*)c->second)[-1]);
    mmap_map.erase(c);
    return result;
  }
  // Don't return -1 as this could be freeing a sub-region, while shrinkage is
  // not supported on z/OS; eventually the block containing this address is
  // expected to be freed.
  return 0;
}

}  // namespace

namespace v8 {
namespace base {

void OS::Free(void* address, const size_t size) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % AllocatePageSize());
  DCHECK_EQ(0, size % AllocatePageSize());
  CHECK_EQ(0, OSFree(address, size));
}

void OS::Release(void* address, size_t size) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % CommitPageSize());
  DCHECK_EQ(0, size % CommitPageSize());
  CHECK_EQ(0, OSFree(address, size));
}

void* OS::Allocate(void* hint, size_t size, size_t alignment,
                   MemoryPermission access) {
  void *reservation = __zalloc(size, alignment);
  if (reservation == nullptr)
    return nullptr;

  std::lock_guard<std::mutex> lock(zalloc_map_mutex);
  zalloc_map[(unsigned long)reservation] = (unsigned long)reservation;
  return reservation;
}

class ZOSTimezoneCache : public PosixTimezoneCache {
  const char* LocalTimezone(double time) override;

  double LocalTimeOffset(double time_ms, bool is_utc) override;

  ~ZOSTimezoneCache() override {}
};

const char* ZOSTimezoneCache::LocalTimezone(double time) {
  if (isnan(time)) return "";
  time_t tv = static_cast<time_t>(std::floor(time / msPerSecond));
  struct tm tm;
  struct tm* t = localtime_r(&tv, &tm);
  if (t == nullptr) return "";
  return tzname[0];
}

double ZOSTimezoneCache::LocalTimeOffset(double time_ms, bool is_utc) {
  time_t tv = time(nullptr);
  struct tm* gmt = gmtime(&tv);
  double gm_secs = gmt->tm_sec + (gmt->tm_min * 60) + (gmt->tm_hour * 3600);
  struct tm* localt = localtime(&tv);
  double local_secs =
      localt->tm_sec + (localt->tm_min * 60) + (localt->tm_hour * 3600);
  return (local_secs - gm_secs) * msPerSecond -
         (localt->tm_isdst > 0 ? 3600 * msPerSecond : 0);
}

TimezoneCache* OS::CreateTimezoneCache() { return new ZOSTimezoneCache(); }


// static
void* OS::AllocateShared(void* hint, size_t size, MemoryPermission access,
                         PlatformSharedMemoryHandle handle, uint64_t offset) {
  DCHECK_EQ(0, size % AllocatePageSize());
  int prot = GetProtectionFromMemoryPermission(access);
  int fd = FileDescriptorFromSharedMemoryHandle(handle);
  void* address = mmap(hint, size, prot, MAP_SHARED, fd, offset);
  if (address == MAP_FAILED) {
    if (gLogMemUsage) {
      __memprintf("ERROR: mmap failed(4), errno=%d, size=%zu, " \
                  "hint=%p, offset==%lu\n", errno, size, hint, offset);
    }
    return nullptr;
  }
  if (gLogMemUsage) {
    __memprintf("addr=%p, mmap OK, size=%zu, hint=%p, " \
                "offset==%lu\n", address, size, hint, offset);
  }
  return address;
}

// static
void OS::FreeShared(void* address, size_t size) {
  DCHECK_EQ(0, size % AllocatePageSize());
  CHECK_EQ(0, munmap(address, size));
  if (gLogMemUsage) {
    __memprintf("addr=%p, munmap OK, size=%zu\n", address, size);
  }
}

bool AddressSpaceReservation::AllocateShared(void* address, size_t size,
                                             OS::MemoryPermission access,
                                             PlatformSharedMemoryHandle handle,
                                             uint64_t offset) {
  DCHECK(Contains(address, size));
  int prot = GetProtectionFromMemoryPermission(access);
  int fd = FileDescriptorFromSharedMemoryHandle(handle);
  void *result = mmap(address, size, prot, MAP_SHARED | MAP_FIXED, fd, offset);
  if (result == MAP_FAILED) {
    if (gLogMemUsage) {
      __memprintf("ERROR: mmap failed(5), errno=%d, hint=%p, size=%zu, " \
                  "offset==%lu\n", errno, address, size, offset);
    }
    return false;
  }
  if (gLogMemUsage) {
    __memprintf("addr=%p, mmap OK, hint=%p, size=%zu, offset==%lu\n",
                result, address, size, offset);
  }
  return true;
}

bool AddressSpaceReservation::FreeShared(void* address, size_t size) {
  DCHECK(Contains(address, size));
  bool rc = mmap(address, size, PROT_NONE, MAP_FIXED | MAP_PRIVATE,
              -1, 0) == address;
  if (!rc) {
    if (gLogMemUsage) {
      __memprintf("ERROR: addr=%p, mmap failed(6), errno=%d, size=%zu\n",
                  address, errno, size);
    }
    return false;
  }
  if (gLogMemUsage) {
    __memprintf("ERROR: addr=%p, mmap (to free) OK, size=%zu\n", address, size);
  }
  return true;
}


std::vector<OS::SharedLibraryAddress> OS::GetSharedLibraryAddresses() {
  std::vector<SharedLibraryAddress> result;
  return result;
}

void OS::SignalCodeMovingGC() {}

void OS::AdjustSchedulingParams() {}

// static
bool OS::SetPermissions(void* address, size_t size, MemoryPermission access) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % CommitPageSize());
  DCHECK_EQ(0, size % CommitPageSize());
  return true;
}

void OS::SetDataReadOnly(void* address, size_t size) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % CommitPageSize());
  DCHECK_EQ(0, size % CommitPageSize());
  // CHECK_EQ(0, mprotect(address, size, PROT_READ));
}

// static
bool OS::RecommitPages(void* address, size_t size, MemoryPermission access) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % CommitPageSize());
  DCHECK_EQ(0, size % CommitPageSize());
  return SetPermissions(address, size, access);
}

// static
bool OS::DiscardSystemPages(void* address, size_t size) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % CommitPageSize());
  DCHECK_EQ(0, size % CommitPageSize());
  return true;
}

// static
bool OS::DecommitPages(void* address, size_t size) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % CommitPageSize());
  DCHECK_EQ(0, size % CommitPageSize());
  return true;
}

// static
bool OS::HasLazyCommits() {
  return false;
}

// All *MemoryMappedFile here were copied from platform-posix.cc for the
// customized z/OS MemoryMappedFile::open()

class PosixMemoryMappedFile final : public OS::MemoryMappedFile {
 public:
  PosixMemoryMappedFile(FILE* file, void* memory, size_t size)
      : file_(file), memory_(memory), size_(size) {}
  ~PosixMemoryMappedFile() final;
  void* memory() const final { return memory_; }
  size_t size() const final { return size_; }

 private:
  FILE* const file_;
  void* const memory_;
  size_t const size_;
};

// static
OS::MemoryMappedFile* OS::MemoryMappedFile::open(const char* name,
                                                 FileMode mode) {
  const char* fopen_mode = (mode == FileMode::kReadOnly) ? "r" : "r+";
  int open_mode = (mode == FileMode::kReadOnly) ? O_RDONLY : O_RDWR;
  // use open() instead of fopen() to prevent auto-conversion
  // (which doesn't support untagged file with ASCII content)
  void *memory = nullptr;
  if (int fd = ::open(name, open_mode)) {
    FILE *file = fdopen(fd, fopen_mode);  // for PosixMemoryMappedFile()
    long size = lseek(fd, 0, SEEK_END);
    if (size == 0) return new PosixMemoryMappedFile(file, nullptr, 0);
    if (size > 0) {
      int prot = PROT_READ;
      int flags = MAP_PRIVATE;
      if (mode == FileMode::kReadWrite) {
        prot |= PROT_WRITE;
        flags = MAP_SHARED;
        memory = mmap(OS::GetRandomMmapAddr(), size, prot, flags, fd, 0);
        if (memory != MAP_FAILED) {
          std::lock_guard<std::mutex> lock(mmap_mutex);
          mmap_map[(unsigned long)memory] = (unsigned long)size;
          __memprintf("addr=%p, size=%zu: open (mmap) OK\n", memory, size);
        } else {
          __memprintf("ERROR: addr=%p, size=%zu: open (mmap) failed, " \
                      "errno=%d\n", memory, size, errno);
        }
      } else {
        memory = __zalloc_for_fd(size, name, fd, 0);
        if (memory != nullptr) {
          std::lock_guard<std::mutex> lock(zalloc_map_mutex);
          zalloc_map[(unsigned long)memory] = (unsigned long)memory;
        }
      }
      return new PosixMemoryMappedFile(file, memory, size);
    } else {
      perror("lseek");
    }
    fclose(file); // also closes fd
  }
  return nullptr;
}

// static
OS::MemoryMappedFile* OS::MemoryMappedFile::create(const char* name,
                                                   size_t size, void* initial) {
  if (FILE* file = fopen(name, "w+")) {
    if (size == 0) return new PosixMemoryMappedFile(file, 0, 0);
    size_t result = fwrite(initial, 1, size, file);
    if (result == size && !ferror(file)) {
      void* memory = mmap(OS::GetRandomMmapAddr(), result,
                          PROT_READ | PROT_WRITE, MAP_SHARED, fileno(file), 0);
      if (memory != MAP_FAILED) {
        std::lock_guard<std::mutex> lock(mmap_mutex);
        mmap_map[(unsigned long)memory] = (unsigned long)size;
        __memprintf("addr=%p, size=%zu: create (mmap) OK\n", memory, size);
        return new PosixMemoryMappedFile(file, memory, result);
      } else {
        __memprintf("addr=%p, size=%zu: create (mmap) failed, errno=%d\n",
                    memory, size, errno);
      }
    }
    fclose(file);
  }
  return nullptr;
}

PosixMemoryMappedFile::~PosixMemoryMappedFile() {
  if (memory_) OS::Free(memory_, RoundUp(size_, OS::AllocatePageSize()));
  fclose(file_);
}

}  // namespace base
}  // namespace v8
