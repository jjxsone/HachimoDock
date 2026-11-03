# HachimoDock 固件 V3 编译说明（本机已验证）

本文记录在本机（Windows）实际配置并成功编译 **v3** 固件的步骤与结果，作为
[BUILD.md](BUILD.md) 的补充。v1 环境按需求不编译。

## 1. 目标与产物

| 项目 | 值 |
| --- | --- |
| 固件版本 | `0.7.103-p4-r1` |
| 构建标识 | `0.7.103-p4-r1+aa9c1371d3e7-dirty` |
| 芯片 | ESP32-P4（v3 芯片，rev 3.0–3.99），400 MHz，32 MiB Flash |
| PlatformIO 环境 | `esp32_p4_evboard_v3` |
| 输出目录 | `firmware/.pio/build/esp32_p4_evboard_v3/` |

关键产物：

| 文件 | 大小 | 用途 |
| --- | --- | --- |
| `firmware.bin` | 2,358,048 B | 应用镜像（日常 OTA / PC 升级） |
| `firmware.factory.bin` | 5,316,608 B | 完整出厂镜像（从 0x0 烧录） |
| `bootloader.bin` | 23,216 B | 引导程序（0x2000） |
| `partitions.bin` | 3,072 B | 分区表（0x8000） |
| `ota_data_initial.bin` | 8,192 B | OTA 元数据（0x510000） |

`firmware.bin` 的 SHA-256：
`01F7503527198A52F95DEEA35DA1CDD838D993A25C1ECCA8753DFEA5C8D442E0`

资源占用：RAM 27.4%（226,844 / 827,392 B），Flash 89.9%（2,356,726 / 2,621,440 B），
OTA 余量 263,392 B（要求 ≥ 131,072 B）。

## 2. 工具链版本

| 组件 | 版本 |
| --- | --- |
| Python | 3.12.10 |
| PlatformIO Core | 6.1.19 |
| pioarduino 平台 | 55.03.38-1 |
| ESP-IDF | 5.5.4 |
| RISC-V 工具链 | 14.2.0+20260121 |
| esptool（编译期） | 5.2.0 |
| Git / Pillow / pytest | 2.55.0 / 13.0.0 / 9.2.0 |

注意：

- 官方要求 PlatformIO Core **6.1.19**；v3 不支持 6.1.18。
- 首次构建需要联网下载 pioarduino 平台、ESP-IDF 与 RISC-V 工具链（工具链约 700 MiB）。
- 出厂烧录包固定使用 esptool **5.4.0**，与编译期 esptool 不同。

## 3. 标准编译步骤

在仓库根目录（`HachimoDock-main`）执行：

```powershell
# 1) 内置素材/组件包完整性检查（本机通过）
python firmware/tools/build_builtin_bundle.py --check

# 2) 安装 Python 依赖（本机已安装）
python -m pip install "platformio==6.1.19" pytest Pillow

# 3) 进入固件目录并编译 v3
cd firmware
python -m platformio run -e esp32_p4_evboard_v3
```

编译成功时末尾会显示：

```text
Environment          Status    Duration
esp32_p4_evboard_v3  SUCCESS   00:09:04.796
```

产物位于 `firmware/.pio/build/esp32_p4_evboard_v3/`。

## 4. 本机环境特殊处理（重要）

本机同时安装了 VSCode 的 PlatformIO 扩展（Core 6.2.0 / Python 3.13）和命令行
PlatformIO（Core 6.1.19 / Python 3.12）。两者共用 `C:\Users\<用户>\.platformio\penv`，
会反复把虚拟环境改成对方的 Python 版本，导致编译在 `Failed to install Python
dependencies into penv` / `uv installation via pip failed` 处失败。

解决办法是为命令行构建使用**独立的 PlatformIO 核心目录**，避免与扩展争用 `penv`：

```powershell
# 一次性创建独立核心目录；packages/platforms/dist 用目录联接复用已有缓存，避免重复下载
$core = 'C:\pio-core'
New-Item -ItemType Directory -Force $core | Out-Null
cmd /c mklink /J "$core\packages"  "$env:USERPROFILE\.platformio\packages"
cmd /c mklink /J "$core\platforms" "$env:USERPROFILE\.platformio\platforms"
cmd /c mklink /J "$core\dist"      "$env:USERPROFILE\.platformio\dist"

# 每次构建前设置（只对当前终端有效）
$env:PLATFORMIO_CORE_DIR = 'C:\pio-core'
cd firmware
python -m platformio run -e esp32_p4_evboard_v3
```

如果不需要和 VSCode 扩展共存，可去掉 `PLATFORMIO_CORE_DIR`，直接使用默认的
`%USERPROFILE%\.platformio`。

其他注意事项：

- 构建目录、临时目录和 PlatformIO 缓存路径应为**纯 ASCII**；本机路径满足。
- v1 与 v3 的镜像不可互刷。v1 平台 `55.03.32` / ESP-IDF `5.5.1` /
  工具链 `14.2.0_20250730` 与 v3 不同，且两个环境不要并行构建。
- v3 的 `sdkconfig` 由 `sdkconfig.defaults` + `sdkconfig.v3.defaults` 生成，
  `firmware/sdkconfig.esp32_p4_evboard_v3` 是构建后的完整配置，不要用 v1 的配置覆盖。
- 构建后 `dependencies.v3.lock` 可能被 ESP-IDF 写入本机路径。提交前运行：

  ```powershell
  python firmware/tools/normalize_dependency_lock.py
  ```

  本次已执行，锁文件保持 `$PET_P4_PROJECT_DIR/components/...` 形式。

## 5. 烧录（COM7 实测）

### 5.1 本机开发板信息

已通过 esptool 5.4.0 在 `COM7` 上读取并确认：

| 项目 | 值 |
| --- | --- |
| 串口 | `COM7` |
| 芯片 | ESP32-P4，revision **v3.2**（400 MHz） |
| Flash | 32 MB（Manufacturer `c8`，Device `4019`） |
| MAC | `e8:f6:0a:e9:81:eb` |
| 烧录工具 | Python 3.12 中的 **esptool 5.4.0**（与官方出厂包一致） |

读取命令（只读，不会写入设备）：

```powershell
python -m esptool --chip esp32p4 --port COM7 --baud 921600 flash-id
```

该板为 v3 芯片，可以烧录本次编译的 v3 镜像（镜像允许 rev 3.0–3.99）。
**不要烧录 v1 镜像**，v1/v3 不可互刷。

### 5.2 烧录前准备

1. 关闭占用 `COM7` 的程序：PC 客户端、串口监视器、PlatformIO Monitor 等。
2. 使用 USB 数据线连接开发板，确认设备管理器中出现 `COM7`。
3. 确认产物存在（在 `firmware` 目录执行）：

   ```powershell
   Get-ChildItem .pio\build\esp32_p4_evboard_v3\firmware*.bin |
     Select-Object Name, Length
   ```

4. 烧录过程中不要拔线、不要断电。

### 5.3 方式 A：保留数据的整包烧录（推荐用于已初始化设备）

适用于设备已经是当前 A/B 分区布局、只想更新固件并保留形象/组件/配置。
按分区写入，不碰 `storage` 和 `appearance` 分区：

```powershell
cd firmware
$b = ".pio\build\esp32_p4_evboard_v3"

python -m esptool --chip esp32p4 --port COM7 --baud 921600 `
  --before default-reset --after hard-reset `
  write-flash -z --flash-mode dio --flash-freq 80m --flash-size 32MB `
  0x2000  "$b\bootloader.bin" `
  0x8000  "$b\partitions.bin" `
  0x510000 "$b\ota_data_initial.bin" `
  0x10000 "$b\firmware.bin"
```

说明：

- `0x2000` Bootloader、`0x8000` 分区表、`0x510000` OTA 元数据、
  `0x10000` 应用镜像，与 `partitions.csv` 一致。
- 写入 `ota_data_initial.bin` 会把启动分区重置为 `ota_0`，这是正常行为。
- 若设备使用旧版单应用/16 MiB 布局，不能直接用本方法，需先按官方指南迁移。

### 5.4 方式 B：首次烧录 / 更新应用（`firmware.factory.bin`，不含存储分区）

适用于全新设备、分区表损坏或需要彻底恢复。使用 `firmware.factory.bin`
从地址 `0x0` 写入，包含 Bootloader、分区表、OTA 元数据和应用程序：

```powershell
cd firmware

python -m esptool --chip esp32p4 --port COM7 --baud 921600 `
  --before default-reset --after hard-reset `
  write-flash -z --flash-mode dio --flash-freq 80m --flash-size 32MB `
  0x0 .pio\build\esp32_p4_evboard_v3\firmware.factory.bin
```

注意：

- 完整烧录可能覆盖设备上已有的配置和素材，操作前先备份。
- 完成后设备会自动复位；首次启动可能需要几秒初始化。
- 官方发行包使用固定 esptool 5.4.0，本机命令行版本正好一致。
- **`firmware.factory.bin` 只有 5,316,608 字节（到 `0x512000` 为止），不包含
  `storage` 和 `appearance` 分区。** 用它烧录的新设备，`/spiffs` 会因分区没有
  文件系统而挂载失败，表现为“组件下载失败 / 一直同步宠物形象”。
  需要预置存储时请用第 8 节的完整出厂镜像。

### 5.5 方式 C：PC 客户端 OTA（日常升级）

设备已能连接 Pet Manager 时，优先使用客户端内的“固件升级”。普通 OTA 只写应用分区，
保留形象、用户组件和配置，无需串口，也不会触发完整烧录。

### 5.6 烧录后验证

1. 观察串口日志（4 Mbaud），启动握手 JSON 中应包含：

   ```json
   "fw":"0.7.103-p4-r1"
   ```

   可用 pyserial 的 miniterm 查看：

   ```powershell
   python -m serial.tools.miniterm COM7 4000000
   ```

   退出：`Ctrl+]`。

2. 或连接 PC 客户端，确认设备版本显示为 `0.7.103-p4-r1`。
3. 若只烧录了应用镜像且版本未变化，检查是否写入了正确的 `0x10000` 地址，
   以及设备是否从另一个 OTA 分区启动。

### 5.7 常见烧录问题

| 现象 | 处理 |
| --- | --- |
| `could not open port 'COM7'` | 端口被客户端/监视器占用；关闭后重试，或拔插 USB。 |
| `Failed to connect` | 按住 BOOT 再点 RST 进入下载模式；确认数据线支持数据传输。 |
| 芯片版本不匹配 | 本板为 v3.2，只能刷 v3 镜像；不要用 `--force` 绕过检查。 |
| 烧录后无法启动 | 断电重启；仍失败则用方式 B 重刷完整镜像。 |
| 校验失败 / 写入中断 | 降低波特率到 `460800` 重试，检查线缆与供电。 |

> `tools/p4.ps1 flash -Port COM7` 也可用，但该脚本默认构建/烧录目标是 **v1**，
> 且会检查分区布局；v3 请使用上面的 esptool 命令或 PC 客户端 OTA。

### 5.8 本次烧录记录（2026-10-30）

按用户要求使用**方式 B（完整出厂镜像）**，实际执行结果：

```text
端口：COM7
芯片：ESP32-P4 revision v3.2，32MB Flash
镜像：.pio\build\esp32_p4_evboard_v3\firmware.factory.bin
大小：5,316,608 字节
SHA-256：DD76F2D2584611C911AC80003C0B3330615094A367773666087B2A6A326CB222
命令：python -m esptool --chip esp32p4 --port COM7 --baud 921600 \
        --before default-reset --after hard-reset \
        write-flash -z --flash-mode dio --flash-freq 80m --flash-size 32MB \
        0x0 .pio\build\esp32_p4_evboard_v3\firmware.factory.bin
```

esptool 5.4.0 输出摘要：

```text
Flash will be erased from 0x00000000 to 0x00511fff...
Compressed 5316608 bytes to 1477170...
Wrote 5316608 bytes (1477170 compressed) at 0x00000000 in 32.6 seconds
Verifying written data...
Hash of data verified.
Hard resetting via RTS pin...
```

烧录后从 `COM7`（4 Mbaud）读取到设备握手，确认启动正常、版本正确：

```json
{"topic":"hello","payload":{"boardDeviceId":"p4-e8f60ae981eb","runtime":"esp-p4","fw":"0.7.103-p4-r1","buildId":"0.7.103-p4-r1+aa9c1371d3e7-dirty","gitSha":"aa9c1371d3e7","buildDirty":true,"protocolSchema":7,"wireProtocol":"pet-usb-jsonl-v3"}}
```

结论：v3 完整镜像烧录成功，数据校验通过，设备已运行 `0.7.103-p4-r1`。

> 烧录期间 `COM7` 曾被残留的 PlatformIO 构建进程占用，导致
> `could not open port 'COM7'`。关闭占用程序（PC 客户端/串口监视器/残留 `pio run`）
> 后重试即可。烧录前建议先确认端口可打开。

## 6. 验证记录

- `python firmware/tools/build_builtin_bundle.py --check`：通过（EXIT=0）。
- v3 编译：成功，耗时 9 分 04 秒（含首次工具链/依赖准备）。
- 主机侧 `pytest`：本机缺少 `cc` 主机编译器，涉及 C 代码编译的测试无法执行；
  纯 Python 测试可通过。如需完整跑测，请安装 MSYS2/MinGW-w64 或 LLVM，
  并确保 `cc` 在 `PATH` 中。
- 镜像版本字符串已在 `firmware.bin` 中确认：`0.7.103-p4-r1`。
- `buildId` 带 `-dirty` 是因为编译时工作区存在未提交改动（`dependencies.v3.lock`
  的本机路径重写与未跟踪的 `.vscode/`），不影响镜像本身。

## 7. 常见问题

| 现象 | 原因 / 处理 |
| --- | --- |
| `uv installation via pip failed with exit code 106` | `penv` 被另一版本 PlatformIO 破坏；删除 `penv` 或使用独立 `PLATFORMIO_CORE_DIR`。 |
| 下载 ESP-IDF / 工具链卡住 | GitHub Release 在国内较慢；可手动下载到 `%USERPROFILE%\.platformio\.cache\downloads\`（文件名为 URL 的 SHA-1），或使用稳定网络后重试。 |
| `Failed to install Python dependencies into penv` | 同上，penv 版本冲突或临时目录不可写。 |
| 编译报 `cc` 找不到 | 仅影响主机侧 C 测试，不影响固件编译；安装 MinGW/MSYS2 后重试。 |
| 上位机一直“同步宠物形象”、下载组件报 `errno=2` | `storage` 分区没有有效 SPIFFS，`/spiffs` 未挂载；按第 8 节烧录完整出厂镜像。 |

## 8. 修复：组件下载失败 / SPIFFS 未挂载（多板通用）

> 适用症状：换上“只烧过 `firmware.factory.bin`”的板子后，上位机一直卡在
> “同步宠物形象”，组件中心下载失败。本文档记录的原因与修复步骤已在
> `2026-11-02` 的 ESP32-P4 v3.2 板（COM6）上验证通过。

### 8.1 现象

| 位置 | 现象 |
| --- | --- |
| 上位机 | 一直显示“同步宠物形象”，组件下载失败 |
| 上位机报错 | `component commit failed: open /spiffs/p4w00.tmp errno=2` |
| 屏幕 | 只有气泡/状态栏，没有宠物形象 |
| 串口日志 | `E SPIFFS: mount failed, -10025`、`E pet-p4: SPIFFS mount failed; preserving storage for explicit recovery` |

### 8.2 根因

1. `errno=2` 是 **ENOENT（路径不存在）**，不是空间不足或权限问题：
   `/spiffs` 这个挂载点根本不存在，所以组件提交时 `open("/spiffs/p4wXX.tmp")` 直接失败。
2. `-10025` 是 ESP-IDF 内部的 **`SPIFFS_ERR_NOT_A_FS`**：
   `storage` 分区（`0x520000`，7.2 MiB）里没有合法的 SPIFFS 超级块/魔数。
3. 固件按设计**不自动格式化**（[`main/pet_p4_main.c`](main/pet_p4_main.c) 的
   `init_storage()`，`format_if_mount_failed = false`）；`protocol.md` 也规定
   损坏/不匹配的卷要保留，只有显式的出厂恢复才能格式化。所以它会一直坏着。
4. 形象清单 `/spiffs/p4/manifest.json`、槽位标记 `p4slot.txt` / `s0_ready`
   同样在 SPIFFS 里，因此日志还会出现
   `P4 asset select failed manifest_len=0 family_count=0`。

**为什么分区会“不是文件系统”**：`pio run` 默认生成的
`firmware.factory.bin` 只有 5,316,608 字节，包含 Bootloader（`0x2000`）、
分区表（`0x8000`）、应用（`0x10000`）、OTA 元数据（`0x510000`），
**到 `0x512000` 就结束了，不包含 `storage` 和 `appearance` 分区**。
只烧这个文件的新板子，`storage` 就是一块没有格式化的裸 flash。

### 8.3 判定（只读，不改设备）

串口（115200 或 4 Mbaud）看启动日志约 1.5 s 处：

```text
E (xxxx) SPIFFS: mount failed, -10025
E (xxxx) pet-p4: SPIFFS mount failed; preserving storage for explicit recovery
```

出现这两行即可确认，无需其它测试。

### 8.4 修复：烧录完整出厂镜像（推荐）

完整出厂镜像由仓库自带的出厂预置工具生成，除应用外还包含
**预置好的 SPIFFS 镜像**和**内置形象槽**。

前置条件（`factory-config.json` 里声明的路径，缺一会直接报错）：

- `../pc/public/terrier-clips/p4-ready/v9-640x480-15fps-225f-h264-crf27/p4/manifest.json`
- `../pc/builtin-clawpkgs/`（29 个内置组件包）

```powershell
cd firmware

# 1) 生成完整出厂镜像（会跑 build_factory_image.py）
python -m platformio run -e esp32_p4_evboard_v3 -t factory

# 2) 确认产物
Get-ChildItem .pio\build\esp32_p4_evboard_v3\pet-manager-p4-factory.bin |
  Select-Object Name, Length
# 期望：23,068,672 字节

# 3) 烧录（COM 口按实际修改）
python -m esptool --chip esp32p4 --port COM6 --baud 460800 `
  --before default-reset --after hard-reset `
  write-flash -z --flash-mode dio --flash-freq 80m --flash-size 32MB `
  0x0 .pio\build\esp32_p4_evboard_v3\pet-manager-p4-factory.bin

# 4) 复位后看日志
python -m serial.tools.miniterm COM6 115200
```

> ⚠ **别烧错文件**：构建目录里有两个名字很像的产物。
> `firmware.factory.bin` 只有 5,316,608 B，**不含存储分区**，烧它等于没修；
> 必须烧 **`pet-manager-p4-factory.bin`（23,068,672 B）**。
> 下命令前用 `Get-ChildItem ... | Select-Object Name,Length` 核对大小。

镜像布局（由 `partitions.csv` 与 `factory-config.json` 决定）：
| 偏移 | 内容 |
| --- | --- |
| `0x2000` | Bootloader |
| `0x8000` | 分区表 |
| `0x10000` | 应用（`ota_0`） |
| `0x510000` | OTA 元数据 |
| **`0x520000`** | **SPIFFS 镜像**（`0x6E0000` = 7.2 MiB，page 512 / block 4096 / 对象名 32 / 元数据 4），预置 29 个内置组件、形象清单、槽位标记 |
| **`0xC00000`** | **内置形象槽 `appearance0`**（`0xA00000` = 10 MiB，`P4RAW01` 头 + H.264 family） |

镜像总长 `0x1600000`，正好在 `appearance1`（自定义形象槽，`0x1600000`）之前结束，
**不会覆盖自定义形象槽 1**。

### 8.5 只修存储分区（保留现有 appearance）

如果板子的 `appearance0` 是好的、只想恢复 `/spiffs`，第 8.4 步的 `-t factory`
同时会在构建目录生成 `default-terrier.spiffs.bin`（正好 `0x6E0000` 字节），
只写这一段即可，速度更快：

```powershell
python -m esptool --chip esp32p4 --port COM6 --baud 460800 `
  --before default-reset --after hard-reset `
  write-flash -z --flash-mode dio --flash-freq 80m --flash-size 32MB `
  0x520000 .pio\build\esp32_p4_evboard_v3\default-terrier.spiffs.bin
```

注意：这会把组件目录重置为出厂 29 项；自定义形象槽 1 不受影响。

### 8.6 验证

启动日志中应满足：

1. **不再出现** `SPIFFS: mount failed`；
2. 出现 `pet-p4-miniapp: restored component catalog count=29 generation=0 sequence=1`；
3. 出现 `pet-p4-miniapp: opened component id=city-radio slot=0`；
4. 上位机能正常下载组件，形象同步不再卡住。

### 8.7 注意

- **破坏性**：会重写 `storage` 分区（组件目录/状态回到出厂）和 `appearance0`；
  自定义形象槽 `appearance1` 不覆盖。需要保留用户数据时先备份。
- 首次启动到组件目录恢复约 **7.5 秒**，期间主任务阻塞、会打印一条
  `task_wdt: Task watchdog got triggered ... IDLE0` 告警。这是告警不是复位，
  系统会继续启动；如果介意可以后续把存储挂载/目录恢复放到独立任务并让出 CPU。
- 普通 OTA / 方式 A 只写应用分区，不会修复存储分区；存储坏掉的板子必须走本节流程。

### 8.8 多板操作清单

对每一块有同样问题的板子：

1. `python -m platformio run -e esp32_p4_evboard_v3 -t factory`（只需一次，产物可复用）；
2. 确认板子串口（设备管理器），关闭上位机/串口监视器；
3. `write-flash -z ... 0x0 pet-manager-p4-factory.bin`（约 5 分钟 @460800）；
4. 复位，确认日志无 `SPIFFS: mount failed` 且有 `restored component catalog count=29`；
5. 打开上位机，下载一个组件、切换一次形象确认正常。

> v1 板子把 `-e esp32_p4_evboard_v3` 换成 `-e esp32_p4_evboard`，并按 v1 的
> `partitions.csv` 确认 `storage` 偏移（不同分区表偏移可能不同，以 `partitions.csv`
> 为准，不要照抄 `0x520000`）。
