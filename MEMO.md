# Memo

## ビルド用環境変数の設定

環境変数 CPPFLAGS`, `LDFLAGS` に標準ライブラリのパスなどを設定する

```sh
$ source ~/osbook/devenv/buildenv.sh
```

## QEMU での起動

```sh
$ ~/osbook/devenv/run_qemu.sh ~/edk2/Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi ~/my_mikanos/kernel/kernel.elf
```

## 実機 PC での起動

1. USB メモリをマウント (USB メモリに対応するデバイスファイルを `/dev/sdX`、マウント先ディレクトリを `/mnt/usb`とする)

```sh
$ sudo mount /dev/sdX /mnt/usb
```

2. 以下のようにファイルを配置

```txt
/mnt/usb
├── kernel.elf ... カーネルのELF
└── EFI
      └── BOOT
            └── BOOTX64.EFI ... ブートローダ
```

3. USB メモリをアンマウントして取り外す

```sh
$ sudo umount /mnt/usb
```

4. 実機 PC に USB メモリを挿して起動
