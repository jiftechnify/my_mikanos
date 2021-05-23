#include "paging.hpp"

#include <array>

#include "asmfunc.h"
#include "memory_manager.hpp"

namespace {
  const uint64_t kPageSize4K = 4096;
  const uint64_t kPageSize2M = 512 * kPageSize4K;
  const uint64_t kPageSize1G = 512 * kPageSize2M;

  alignas(kPageSize4K) std::array<uint64_t, 512> pml4_table;
  alignas(kPageSize4K) std::array<uint64_t, 512> pdp_table;
  alignas(kPageSize4K) std::array<std::array<uint64_t, 512>, kPageDirectoryCount> page_directory; 
}

// リニアアドレスと物理アドレスが一致するようなページテーブルを設定する
// (アイデンティティマッピング)
void SetupIdentityPageTable() {
  pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table[0]) | 0x003;
  for (int i_pdpt = 0; i_pdpt < page_directory.size(); ++i_pdpt) {
    pdp_table[i_pdpt] = reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
    for (int i_pd = 0; i_pd < 512; ++i_pd) {
      page_directory[i_pdpt][i_pd] = i_pdpt * kPageSize1G + i_pd * kPageSize2M | 0x083;
    }
  } 
  SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0]));
}

void InitializePaging() {
  SetupIdentityPageTable();
}

void ResetCR3() {
  SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0]));
}

WithError<PageMapEntry*> NewPageMap() {
  auto frame = memory_manager->Allocate(1);
  if (frame.error) {
    return { nullptr, frame.error };
  }

  auto e = reinterpret_cast<PageMapEntry*>(frame.value.Frame());
  memset(e, 0, sizeof(uint64_t) * 512);
  return { e, MAKE_ERROR(Error::kSuccess) };
}

WithError<PageMapEntry*> SetNewPageMapIfNotPresent(PageMapEntry& entry) {
  if (entry.bits.present) {
    // 引数のエントリが既にどこかを指している場合は何もしない
    return { entry.Pointer(), MAKE_ERROR(Error::kSuccess) };
  }

  auto [ child_map, err ] = NewPageMap();
  if (err) {
    return { nullptr, err };
  }

  entry.SetPointer(child_map);
  entry.bits.present = 1;

  return { child_map, MAKE_ERROR(Error::kSuccess) };
}

WithError<size_t> SetupPageMap(PageMapEntry* page_map, int page_map_level, LinearAddress4Level addr, size_t num_4kpages) {
  while (num_4kpages > 0) {
    const auto entry_index = addr.Part(page_map_level);

    auto [ child_map, err ] = SetNewPageMapIfNotPresent(page_map[entry_index]);
    if (err) {
      return { num_4kpages, err };
    }
    page_map[entry_index].bits.writable = 1;
    page_map[entry_index].bits.user = 1;  // アプリが動作する際のCPU動作権限レベルでも命令をフェッチできるようにする

    if (page_map_level == 1) { // PT
      --num_4kpages;
    } else {
      auto [ num_remain_pages, err ] = SetupPageMap(child_map, page_map_level - 1, addr, num_4kpages);
      if (err) {
        return { num_4kpages, err };
      }
      num_4kpages = num_remain_pages;
    }
    
    if (entry_index == 511) {
      // テーブルがいっぱいになった
      break;
    }

    // 次のページを割り当てるエントリを指す仮想アドレスを求める
    addr.SetPart(page_map_level, entry_index + 1);
    for (int level = page_map_level - 1; level >= 1; --level) {
      addr.SetPart(level, 0);
    }
  }
  return { num_4kpages, MAKE_ERROR(Error::kSuccess) };
}

Error SetupPageMaps(LinearAddress4Level addr, size_t num_4kpages) {
  auto pml4_table = reinterpret_cast<PageMapEntry*>(GetCR3());
  return SetupPageMap(pml4_table, 4, addr, num_4kpages).error;
}

const FileMapping* FindFileMapping(const std::vector<FileMapping>& fmaps, uint64_t causal_vaddr) {
  for (const FileMapping& m : fmaps) {
    if (m.vaddr_begin <= causal_vaddr && causal_vaddr < m.vaddr_end) {
      return &m;
    }
  }
  return nullptr;
}

// メモリマップトファイル用のページを作成し、そこにファイルの中身をコピー
Error PreparePageCache(FileDescriptor& fd, const FileMapping& m, uint64_t causal_vaddr) {
  LinearAddress4Level page_vaddr{causal_vaddr};
  page_vaddr.parts.offset = 0;
  if (auto err = SetupPageMaps(page_vaddr, 1)) {
    return err;
  }

  const long file_offset = page_vaddr.value - m.vaddr_begin;
  void* page_cache = reinterpret_cast<void*>(page_vaddr.value);
  fd.Load(page_cache, 4096, file_offset);
  return MAKE_ERROR(Error::kSuccess);
}

Error HandlePageFault(uint64_t error_code, uint64_t causal_addr) {
  auto& task = task_manager->CurrentTask();

  if (error_code & 1) { // 権限違反による例外
    return MAKE_ERROR(Error::kAlreadyAllocated);
  }
  if (task.DPagingBegin() <= causal_addr && causal_addr < task.DPagingEnd()) {  // デマンドページング用の領域の場合
    return SetupPageMaps(LinearAddress4Level{causal_addr}, 1);
  } 
  if (auto m = FindFileMapping(task.FileMaps(), causal_addr)) { // 予約済みのファイルマッピング領域の場合
    return PreparePageCache(*task.Files()[m->fd], *m, causal_addr);
  }
  return MAKE_ERROR(Error::kIndexOutOfRange);
}

