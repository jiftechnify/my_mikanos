#include "pci.hpp"

#include "asmfunc.h"

namespace {
  using namespace pci;

  uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr) {
    auto shl = [](uint32_t x, unsigned int bits) {
      return x << bits;
    };

    return shl(1, 31) // enable bit
         | shl(bus, 16)
         | shl(device, 11)
         | shl(function, 8)
         | (reg_addr & 0xfcu); // 下位2bitを0に
  }

  // devices にデバイス情報を追加
  Error AddDevice(const Device& device) {
    if (num_device == devices.size()) {
      return MAKE_ERROR(Error::kFull);
    }

    devices[num_device] = device;
    ++num_device;
    return MAKE_ERROR(Error::kSuccess);
  }

  Error ScanBus(uint8_t bus);

  // 指定ファンクションを devices に追加する
  // もし PCI-PCI ブリッジなら、さらにセカンダリバスを探索する
  Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
    auto class_code = ReadClassCode(bus, device, function);
    auto header_type = ReadHeaderType(bus, device, function);
    Device dev{bus, device, function, header_type, class_code};
    if (auto err = AddDevice(dev)) {
      return err;
    }

    if (class_code.Match(0x06u, 0x04u)) {
      // standard PCI-PCI bridge
      auto bus_numbers = ReadBusNumbers(bus, device, function);
      uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
      return ScanBus(secondary_bus);
    }

    return MAKE_ERROR(Error::kSuccess);
  }

  // 指定デバイスの全ファンクションをスキャン
  Error ScanDevice(uint8_t bus, uint8_t device) {
    // ファンクション0は必ず存在するはずなので、最初にスキャン
    if (auto err = ScanFunction(bus, device, 0)) {
      return err;
    }
    // 単一ファンクションデバイスなら打ち切る
    if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
      return MAKE_ERROR(Error::kSuccess);
    }

    // マルチファンクションデバイスなら、さらに有効なファンクションを探す
    for (uint8_t function = 1; function < 8; ++function) {
      if (ReadVendorId(bus, device, function) == 0xffffu) {
        continue;
      }
      if (auto err = ScanFunction(bus, device, function)) {
        return err;
      }
    }
    return MAKE_ERROR(Error::kSuccess);
  }

  // 指定バス上の全デバイスをスキャン
  Error ScanBus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; ++device) {
      // ファンクション0のベンダIDが無効値(0xffff)の場合、デバイスは存在しないということ
      if (ReadVendorId(bus, device, 0) == 0xffffu) {
        continue;
      }
      if (auto err = ScanDevice(bus, device)) {
        return err;
      }
    }
    return MAKE_ERROR(Error::kSuccess);
  }
}

namespace pci {
  // CONFIG_ADDRESS レジスタに書き込む
  void WriteAddress(uint32_t address) {
    IoOut32(kConfigAddress, address);
  }

  // CONFIG_DATA レジスタに書き込む
  void WriteData(uint32_t value) {
    IoOut32(kConfigData, value);
  }

  // CONFIG_DATA レジスタを読む
  uint32_t ReadData() {
    return IoIn32(kConfigData);
  }

  // PCIコンフィギュレーション空間からVendor IDを読む
  uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
    // PCIコンフィギュレーション空間の先頭32bitを読み取る
    WriteAddress(MakeAddress(bus, device, function, 0x00));
    // そのうち後半16bitがVendor ID
    return ReadData() & 0xffffu;
  }

  /** @brief デバイス ID レジスタを読み取る（全ヘッダタイプ共通） */
  uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x00));
    return ReadData() >> 16;
  }
  /** @brief ヘッダタイプレジスタを読み取る（全ヘッダタイプ共通） */
  uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x0c));
    return (ReadData() >> 16) & 0xffu;
  }
  /** @brief クラスコードレジスタを読み取る（全ヘッダタイプ共通） */
  ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x08));
    auto reg = ReadData();
    ClassCode cc;
    cc.base      = (reg >> 24) & 0xffu;
    cc.sub       = (reg >> 16) & 0xffu;
    cc.interface = (reg >> 8) & 0xffu;
    return cc;
  }

  uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x18));
    return ReadData();
  }

  // 単一ファンクションデバイスか? ヘッダタイプの bit 7 で判別
  bool IsSingleFunctionDevice(uint8_t header_type) {
    return (header_type & 0x80u) == 0;
  }

  // PCIデバイスをすべて探索
  Error ScanAllBus() {
    num_device = 0;

    // ホストブリッジのヘッダタイプを読み取る
    auto header_type = ReadHeaderType(0, 0, 0);
    if (IsSingleFunctionDevice(header_type)) {
      return ScanBus(0);
    }

    for (uint8_t function = 1; function < 8; ++function) {
      if (ReadVendorId(0, 0, function) == 0xffffu) {
        continue;
      }
      if (auto err = ScanBus(function)) {
        return err;
      }
    }
    return MAKE_ERROR(Error::kSuccess);
  }
}
