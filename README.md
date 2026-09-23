[![Forgejo CI](https://forgejo.acnas.net/app/msicontroller/actions/workflows/ci.yml/badge.svg?branch=main)](https://forgejo.acnas.net/app/msicontroller/actions/workflows/ci.yml/badge.svg?branch=main)

# MSI Control Center

**MSI Control Center** is a Linux control panel for supported MSI laptops. It combines a Qt 6 desktop client, a system D-Bus service, a helper library, and an EC kernel module for monitoring thermals and controlling laptop-specific features.

DEB/RPM packages bundle Qt under `/opt/msicontroller`; the Arch package uses system Qt under `/usr`. All packages install the same client, system service, and DKMS kernel module.

## Features

- Live temperatures, fan speeds, battery status, and CPU/GPU controls.
- Cooling modes, fan curves, shift modes, keyboard backlight, and battery charge settings where supported by firmware.
- Built-in and editable MSI device profiles, with JSON import and export.
- Diagnostics, a support bundle, and the `msicontroller-doctor` command.
- Russian translation, desktop launcher, system tray, and a fake EC backend for development.

Available controls depend on the detected firmware profile in `src/service/supported-devices.json`.

## Screenshots

**Dashboard and cooling controls**

![Dashboard and cooling controls](screenshot/Screenshot_20260601_081953.png)

**CPU performance and device controls**

![CPU performance and device controls](screenshot/Screenshot_20260601_082011.png)

**Supported device profiles**

![Supported device profiles](screenshot/Screenshot_20260601_082024.png)

**EC memory debugger**

![EC memory debugger](screenshot/Screenshot_20260601_082031.png)

## Architecture

The Qt/QML client talks to a privileged system D-Bus service. The service owns EC access and validates raw writes against the active device profile. DKMS builds the EC kernel module for installed kernels with matching headers.

## Installation

Download the package for your distribution from a [Forgejo release](https://forgejo.acnas.net/app/msicontroller/releases) or CI artifacts. Install headers matching your kernel first; on Omarchy use the headers for its kernel, not the stock `linux-headers` package.

| Distribution | Install downloaded package |
| --- | --- |
| Debian / Ubuntu | `sudo apt install ./msicontroller_amd64.deb` |
| Fedora / RPM | `sudo dnf install ./msicontroller_x86_64.rpm` |
| Arch / Omarchy | `sudo pacman -U ./msicontroller-*.pkg.tar.zst` |

The packages set up the signed Forgejo repository for future updates. On Arch, the package checks the bundled public key before trusting it and removes its repository entry when uninstalled. No manual `pacman.conf` edit is needed.

The service is enabled and started during package installation; on Arch it starts after DKMS finishes.

Update as usual with `sudo apt update && sudo apt upgrade`, `sudo dnf upgrade`, or `sudo pacman -Syu`. Launch **MSI Control Center** from the application menu. If DKMS fails, check that your running kernel has matching headers and run `dkms status -m msiecmodule`.

## Building and Testing

- DEB/RPM: run `./scripts/build-packages-docker.sh` to build with Qt 6.11.1 in Docker. Packages are written to `packages/`.
- Arch: install the build dependencies in [`PKGBUILD.in`](cmake/packaging/arch/PKGBUILD.in), then run `./scripts/build-arch-package.sh` as a regular user. The package is written to `packages/`.
- Package install tests: run `./scripts/test-packages-docker.sh`.
- Unit tests: run `ctest --test-dir build --output-on-failure` after a CMake build.

Forgejo CI builds and tests all three package formats. Tagged releases publish the packages to the Forgejo registry, so later versions arrive through the normal package manager.

For a source build on Debian/Ubuntu, install Qt 6 development packages, CMake, Ninja, D-Bus and systemd development files, and matching kernel headers, then run:

```sh
cmake -S . -B build -G Ninja
cmake --build build
sudo cmake --install build
```

For diagnostics, run `systemctl status msi-ec-service`. The client and doctor binaries are under `/opt/msicontroller/bin/` on DEB/RPM and `/usr/bin/` on Arch.

## Supported Laptops

The current firmware configuration database contains 56 built-in MSI firmware profiles (`CONF0` through `CONF55`). Feature availability is firmware-specific; the table below is based on `src/service/supported-devices.json`.

| Config  | Firmware(s) | Fan | Shift | Keyboard Backlight | Super Battery | Mic Mute |
|---------|-------------|-----|-------|--------------------|---------------|----------|
| CONF0   | 14C1EMS1.012, 14C1EMS1.101, 14C1EMS1.102 | ✔ | ✔ | ✔ |  | ✔ |
| CONF1   | 17F2EMS1.103, 17F2EMS1.104, 17F2EMS1.106, 17F2EMS1.107 | ✔ | ✔ | ✔ |  | ✔ |
| CONF2   | 1552EMS1.115, 1552EMS1.118, 1552EMS1.119, 1552EMS1.120 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF3   | 1592EMS1.111 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF4   | 16V4EMS1.114, 16V4EMS1.115 | ✔ | ✔ |  |  |  |
| CONF5   | 158LEMS1.103, 158LEMS1.105, 158LEMS1.106 | ✔ | ✔ |  |  | ✔ |
| CONF6   | 1542EMS1.102, 1542EMS1.104 | ✔ | ✔ |  | ✔ |  |
| CONF7   | 17FKEMS1.108, 17FKEMS1.109, 17FKEMS1.10A | ✔ | ✔ | ✔ |  |  |
| CONF8   | 14F1EMS1.114, 14F1EMS1.115, 14F1EMS1.116, 14F1EMS1.117, 14F1EMS1.118, 14F1EMS1.119, 14F1EMS1.120 | ✔ | ✔ | ✔ | ✔ |  |
| CONF9   | 14JKEMS1.104, 14JKEMS1.300 | ✔ | ✔ | ✔ |  | ✔ |
| CONF10  | 1582EMS1.107 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF11  | 16S6EMS1.111 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF12  | 16R6EMS1.104, 16R6EMS1.106, 16R6EMS1.107 | ✔ | ✔ | ✔ |  |  |
| CONF13  | 1594EMS1.109 | ✔ | ✔ | ✔ |  | ✔ |
| CONF14  | 17L2EMS1.108 | ✔ | ✔ | ✔ |  | ✔ |
| CONF15  | 15CKEMS1.108 | ✔ | ✔ | ✔ |  | ✔ |
| CONF16  | 155LEMS1.105, 155LEMS1.106 | ✔ | ✔ | ✔ |  | ✔ |
| CONF17  | 15K1IMS1.110, 15K1IMS1.111, 15K1IMS1.112, 15K1IMS1.113 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF18  | 15HKEMS1.104 | ✔ | ✔ | ✔ |  | ✔ |
| CONF19  | 1543EMS1.113, 17K3EMS1.115 | ✔ | ✔ |  | ✔ |  |
| CONF20  | 1581EMS1.107 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF21  | 16R3EMS1.100, 16R3EMS1.102, 16R3EMS1.104, 16R4EMS2.102 | ✔ | ✔ | ✔ |  |  |
| CONF22  | 17LLEMS1.106 | ✔ | ✔ |  |  | ✔ |
| CONF23  | 16WKEMS1.105 | ✔ | ✔ | ✔ |  |  |
| CONF24  | 14D1EMS1.103 | ✔ | ✔ | ✔ |  | ✔ |
| CONF25  | 14F1EMS1.207, 14F1EMS1.209, 14F1EMS1.211 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF26  | 14DLEMS1.105 | ✔ | ✔ | ✔ |  | ✔ |
| CONF27  | 17S2IMS1.113 | ✔ | ✔ |  | ✔ | ✔ |
| CONF28  | 1822EMS1.105, 1822EMS1.109, 1822EMS1.111, 1822EMS1.112, 1822EMS1.114, 1822EMS1.115 | ✔ | ✔ |  | ✔ | ✔ |
| CONF29  | 16V5EMS1.107 | ✔ | ✔ |  | ✔ |  |
| CONF30  | 17Q2IMS1.107, 17Q2IMS1.10D | ✔ | ✔ | ✔ |  |  |
| CONF31  | 16Q4EMS1.110 | ✔ | ✔ |  |  |  |
| CONF32  | 158PIMS1.207, 158PIMS1.112, 158PIMS1.114 | ✔ | ✔ | ✔ |  | ✔ |
| CONF33  | 17N1EMS1.109 | ✔ | ✔ |  | ✔ | ✔ |
| CONF34  | 14C6EMS1.109 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF35  | 15M2IMS1.113 | ✔ | ✔ |  | ✔ | ✔ |
| CONF36  | 1585EMS1.112, 1585EMS1.115 | ✔ | ✔ |  | ✔ | ✔ |
| CONF37  | 15M1IMS1.113 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF38  | 17E8IMS1.106, 17E8EMS1.101 | ✔ | ✔ | ✔ |  |  |
| CONF39  | 16R8IMS1.108, 16R8IMS1.111, 16R8IMS1.117 | ✔ | ✔ | ✔ | ✔ |  |
| CONF40  | 17S1IMS1.105 | ✔ | ✔ |  | ✔ | ✔ |
| CONF41  | 15M1IMS2.111 | ✔ | ✔ |  |  | ✔ |
| CONF42  | 14L1EMS1.307, 14L1EMS1.308 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF43  | 14DKEMS1.104, 14DKEMS1.105 | ✔ | ✔ | ✔ |  | ✔ |
| CONF44  | 17LNIMS1.505 | ✔ | ✔ |  | ✔ | ✔ |
| CONF45  | 1545IMS1.109 | ✔ | ✔ |  | ✔ |  |
| CONF46  | 16W1EMS1.102, 16W1EMS1.103, 16W1EMS1.104, 16W1EMS2.103, 16W2EMS1.101 | ✔ | ✔ | ✔ |  |  |
| CONF47  | 17KKIMS1.108, 17KKIMS1.109, 17KKIMS1.114 | ✔ | ✔ |  |  |  |
| CONF48  | 158NIMS1.10D, 158NIMS1.30C | ✔ | ✔ | ✔ |  | ✔ |
| CONF49  | 16R4EMS1.101, 16R4EMS1.102, 16R5EMS1.102 | ✔ | ✔ | ✔ |  |  |
| CONF50  | 1584EMS1.112 | ✔ | ✔ | ✔ | ✔ | ✔ |
| CONF51  | 158MEMS1.101, 158KEMS1.104, 158KEMS1.106, 158KEMS1.109 | ✔ | ✔ | ✔ |  | ✔ |
| CONF52  | 16V1EMS1.109, 16V1EMS1.116, 16V1EMS1.118, 16V3EMS1.106 | ✔ | ✔ |  |  |  |
| CONF53  | 159KIMS1.108 | ✔ | ✔ | ✔ |  | ✔ |
| CONF54  | 16R8IMS2.112 | ✔ | ✔ | ✔ | ✔ |  |
| CONF55  | 17G1EMS1.107 | ✔ | ✔ |  |  |  |

Built-in profiles are shipped in `src/service/supported-devices.json`. User profiles and overrides are written by the service to `/etc/MsiController/supported-devices.json` and can be managed from the client via **Supported devices** in the left panel. Profiles can also be imported from and exported to JSON files from the same page. Saved profile changes are applied live: the service rebuilds the active EC parameter set, refreshes the raw-write safety allowlist, and the client refreshes without restarting the service. User profile files are schema-versioned for forward-compatible migrations.

### Regenerating Device Profiles

The built-in JSON database can be regenerated manually from the C configurations in [BeardOverflow/msi-ec](https://github.com/BeardOverflow/msi-ec):

```sh
scripts/generate-supported-devices-from-msi-ec.py --dry-run > /tmp/supported-devices.json
scripts/generate-supported-devices-from-msi-ec.py --output src/service/supported-devices.json
```

To use an already cloned upstream repository:

```sh
scripts/generate-supported-devices-from-msi-ec.py --source-dir /path/to/msi-ec --output src/service/supported-devices.json
```

The script is not part of the application runtime. It parses upstream `msi-ec.c` / `ec_memory_configuration.h`, keeps MsiController-only defaults for battery status and USB power share, and writes explicit EC addresses into every generated profile so unsupported upstream fields do not fall back to stale defaults.

## Contact / Support

- Email: mikhail@acnas.net  
- Telegram: [@AcNasRu](https://t.me/AcNasRu)
- LinkedIn: [Mikhail Bulatov](https://www.linkedin.com/in/mikhail-bulatov-13277a253/)

## Acknowledgements

Special thanks to [BeardOverflow/msi-ec](https://github.com/BeardOverflow/msi-ec) for inspiration.

## License

This project is licensed under the MIT License.

**Note:** This project uses the Qt framework, which is licensed under the GNU Lesser General Public License (LGPL) version 3.  
See [https://www.qt.io/licensing](https://www.qt.io/licensing) and the `LICENSE.LGPL3` file for details.

```
MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
