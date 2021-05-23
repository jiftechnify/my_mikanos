#pragma once

#include "file.hpp"
#include <cstdint>
#include <cstddef>
#include <utility>

namespace fat {
  // BIOS Parameter Block
  struct BPB {
    uint8_t jump_boot[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
  } __attribute__((packed));

  enum class Attribute : uint8_t {
    kReadOnly  = 0x01,
    kHidden    = 0x02,
    kSystem    = 0x04,
    kVolumeID  = 0x08,
    kDirectory = 0x10,
    kArchive   = 0x20,
    kLongName  = 0x0f,
  };

  struct DirectoryEntry {
    unsigned char name[11];
    Attribute attr;
    uint8_t ntres;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;

    uint32_t FirstCluster() const {
      return first_cluster_low |
        (static_cast<uint32_t>(first_cluster_high) << 16);
    }
  } __attribute__((packed));

  extern BPB* boot_volume_image;
  extern unsigned long bytes_per_cluster;

  void Initialize(void* volume_image);

  // 指定されたクラスタの先頭セクタのメモリアドレス
  uintptr_t GetClusterAddr(unsigned long cluster);

  // 指定されたクラスタの先頭セクタがあるメモリ領域を返す
  template <class T>
  T* GetSectorByCluster(unsigned long cluster) {
    return reinterpret_cast<T*>(GetClusterAddr(cluster));
  }
  
  // ディレクトリエントリの短名を、基本名と拡張子に分割して取得する
  void ReadName(const DirectoryEntry& entry, char* base, char* ext);

  // ディレクトリエントリの短名を整形する
  void FormatName(const DirectoryEntry& entry, char* dest);

  // クラスタ末尾を表す特殊クラスタ番号
  static const unsigned long kEndOfClusterchain = 0x0ffffffflu;

  // クラスタチェーン(1つのファイルを成すクラスタの連結リスト)における、あるクラスタの次のクラスタの番号を取得
  unsigned long NextCluster(unsigned long cluster);

  // クラスタ番号で指定されたディレクトリ内の特定の名前のファイルを指すディレクトリエントリ
  std::pair<DirectoryEntry*, bool> FindFile(const char* name, unsigned long directory_cluster = 0);

  // ファイル名が一致するか
  bool NameIsEqual(const DirectoryEntry& entry, const char* name);

  // buf に entry が指すファイルの内容を読み込む
  size_t LoadFile(void* buf, size_t len, const DirectoryEntry& entry);

  class FileDescriptor : public ::FileDescriptor {
    public:
      explicit FileDescriptor(DirectoryEntry& fat_entry);
      size_t Read(void* buf, size_t len) override;

    private:
      DirectoryEntry& fat_entry_;
      size_t rd_off_ = 0;             // ファイル先頭からの読み込み位置のオフセット
      unsigned long rd_cluster_ = 0;  // rd_off_が指す位置のクラスタ番号
      size_t rd_cluster_off_ = 0;     // クラスタ先頭からのオフセット
  }; 
} // namespace fat

