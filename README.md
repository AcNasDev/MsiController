[![Forgejo CI](https://forgejo.acnas.net/app/msicontroller/actions/workflows/ci.yml/badge.svg?branch=main)](https://forgejo.acnas.net/app/msicontroller/actions/workflows/ci.yml/badge.svg?branch=main)

# MSI Control Center

**MSI Control Center** is a Linux control panel for supported MSI laptops. It combines a Qt 6 desktop client, a system D-Bus service, a helper library, and an EC kernel module for monitoring thermals and controlling laptop-specific features.

The packaged build is designed for daily use: the application is installed into `/opt/msicontroller`, Qt runtime files are bundled privately, and the kernel module is managed through DKMS so it is rebuilt when the kernel changes.

## Features

- Dashboard with CPU/GPU temperatures, fan speed, battery state, and live GPU-rendered mini charts.
- Cooling modes: firmware auto, manual fan curve, target temperature, and Cooler Boost.
- Service-managed target temperature mode for CPU/GPU fan adjustment.
- Editable CPU and GPU fan curves with temperature-to-speed maps.
- CPU performance view for per-core frequency, usage, and frequency limits.
- CPU controls for frequency limit and governor selection.
- Shift mode switching where firmware supports it: Eco, Comfort, Sport, Turbo.
- Device controls for webcam, USB Power Share, FN/Meta swap, Super Battery, mute state, LEDs, keyboard backlight, and battery charge threshold where available.
- Multiple UI themes, desktop entry, autostart entry, and system tray integration.
- System service over D-Bus with systemd integration.
- DEB/RPM packaging with DKMS-managed kernel module installation.
- Forgejo CI package artifacts for releases and test builds.

Feature availability depends on the detected firmware configuration in `src/service/settings.ini`.

## Screenshots

**Dashboard and cooling controls**

![Dashboard and cooling controls](screenshot/Screenshot_20260530_114620.png)

**CPU performance and device controls**

![CPU performance and device controls](screenshot/Screenshot_20260530_114644.png)

## Architecture

- `src/client` - Qt/QML desktop client (`MsiControlCenterClient`).
- `src/service` - privileged system service (`MsiControlCenterService`) exposing device state and commands over D-Bus.
- `src/helper` - shared D-Bus/helper library used by the client and service.
- `src/module` - EC kernel module and DKMS source files.
- `cmake/packaging` - DEB/RPM, DKMS, Docker, and Qt runtime bundling helpers.
- `scripts` - package build entry points for local and Docker builds.

The client does not talk to EC hardware directly. It talks to the service, and the service owns hardware access, CPU control readback, fan target control, and state synchronization.

## Installation

### DEB

```sh
sudo apt install ./packages/msicontroller_amd64.deb
```

### RPM

```sh
sudo dnf install ./packages/msicontroller_x86_64.rpm
```

or:

```sh
sudo rpm -Uvh ./packages/msicontroller_x86_64.rpm
```

The install host needs DKMS, `kmod`, systemd, and kernel headers for the running kernel. The package installs application files into `/opt/msicontroller` and system integration files into standard locations:

- systemd service: `/lib/systemd/system/msi-ec-service.service`
- D-Bus policy: `/etc/dbus-1/system.d/msi-ec-service.conf`
- DKMS source: `/usr/src/msiecmodule-<version>`
- modules-load config: `/etc/modules-load.d/msiecmodule.conf`
- desktop/autostart entries: `/usr/share/applications` and `/etc/xdg/autostart`

After installation:

```sh
systemctl status msi-ec-service
/opt/msicontroller/bin/msicontroller-client
```

The desktop launcher is named **MSI Control Center**.

## Building Packages

The recommended packaging path is Docker. It builds inside Ubuntu 22.04 with Qt 6.11.1 from `aqtinstall`, bundles the Qt runtime, and writes stable artifact names into `packages/`.

```sh
./scripts/build-packages-docker.sh
```

Generated files:

- `packages/msicontroller_amd64.deb`
- `packages/msicontroller_x86_64.rpm`

The filenames stay stable, but package release metadata is unique so upgrades work normally. The script keeps the `packages/` directory in place and atomically replaces package files, which avoids breaking terminals opened inside that directory.

If your build environment needs a proxy:

```sh
export MSICONTROLLER_HTTP_PROXY="http://user:password@proxy.example:3128"
./scripts/build-packages-docker.sh
```

To build packages without Docker, install Qt 6.11.1 locally and run:

```sh
export MSICONTROLLER_QT_VERSION=6.11.1
export MSICONTROLLER_QT_HOST_DIR=/opt/Qt/6.11.1/gcc_64
./scripts/build-packages.sh
```

## Forgejo CI

Forgejo builds the project on pushes to `main` and has a dedicated package job. Download ready-to-install packages from the workflow artifact named `msicontroller-packages`.

When a tag matching `v*` is pushed, the package job also creates or updates the matching Forgejo Release and uploads:

- `msicontroller_amd64.deb`
- `msicontroller_x86_64.rpm`

The CI package job supports the same proxy variable:

```sh
MSICONTROLLER_HTTP_PROXY
```

## Building From Source

Install development dependencies on Debian/Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build pkg-config git \
  qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools \
  qt6-charts-dev qt6-qmltooling-plugins \
  libdbus-1-dev libsystemd-dev \
  linux-headers-$(uname -r)
```

Build:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Install from the source build:

```sh
sudo cmake --install build
```

Uninstall a source install:

```sh
sudo cmake --build build --target uninstall
```

For package-style builds, the module is not compiled during CMake build; DKMS compiles it during package installation and on kernel updates:

```sh
cmake -S . -B build-package -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/msicontroller \
  -DMSICONTROLLER_BUNDLE_QT_RUNTIME=ON \
  -DMSICONTROLLER_BUILD_KERNEL_MODULE=OFF \
  -DMSICONTROLLER_INSTALL_DKMS=ON
```

## Runtime Commands

```sh
systemctl status msi-ec-service
journalctl -u msi-ec-service -f
/opt/msicontroller/bin/msicontroller-client
```

For a non-packaged source install, the client binary is usually available as:

```sh
MsiControlCenterClient
```

## Supported Laptops

The current firmware configuration database contains 56 supported MSI firmware profiles (`CONF0` through `CONF55`). Feature availability is firmware-specific; the table below is based on `src/service/settings.ini`.

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

*For details and updates, see `src/service/settings.ini`.*

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
