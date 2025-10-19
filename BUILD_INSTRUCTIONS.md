# CueForge Qt6 - Detailed Build Instructions

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Platform-Specific Setup](#platform-specific-setup)
3. [Building the Project](#building-the-project)
4. [Troubleshooting](#troubleshooting)
5. [Advanced Options](#advanced-options)

---

## Prerequisites

### Required Software
- **Qt 6.5.0 or later**
- **CMake 3.20 or later**
- **C++17 compiler**
- **Git** (for cloning)

### Required Qt Modules
The following Qt6 modules are required:
- Qt6::Core
- Qt6::Gui
- Qt6::Widgets
- Qt6::Network
- Qt6::Multimedia
- Qt6::MultimediaWidgets
- Qt6::SerialPort

---

## Platform-Specific Setup

### macOS

#### Install Dependencies
```bash
# Install Homebrew (if not installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Qt6
brew install qt@6

# Install CMake
brew install cmake

# Add Qt to PATH
echo 'export PATH="/usr/local/opt/qt@6/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

#### Verify Installation
```bash
qmake --version  # Should show Qt 6.x
cmake --version  # Should show 3.20+
```

### Linux (Ubuntu/Debian)

#### Install Dependencies
```bash
# Update package list
sudo apt-get update

# Install Qt6 development packages
sudo apt-get install -y \
    qt6-base-dev \
    qt6-multimedia-dev \
    libqt6serialport6-dev \
    cmake \
    build-essential \
    git

# Optional: Install Qt Creator
sudo apt-get install qtcreator
```

#### Verify Installation
```bash
qmake6 --version
cmake --version
gcc --version
```

### Linux (Fedora/RHEL)
```bash
# Install dependencies
sudo dnf install -y \
    qt6-qtbase-devel \
    qt6-qtmultimedia-devel \
    qt6-qtserialport-devel \
    cmake \
    gcc-c++ \
    make

# Verify
cmake --version
gcc --version
```

### Windows

#### Option 1: Qt Online Installer (Recommended)
1. Download Qt Online Installer from https://www.qt.io/download
2. Run installer
3. Select **Qt 6.5.x** for your compiler:
   - MSVC 2019/2022 (recommended)
   - MinGW 11.2.0
4. Select these components:
   - Qt 6.5.x Desktop
   - Qt Multimedia
   - Qt SerialPort
   - Qt Creator (optional)

#### Option 2: Package Manager (vcpkg)
```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install Qt6
.\vcpkg install qt6:x64-windows
```

#### Install CMake
- Download from https://cmake.org/download/
- Or use Chocolatey: `choco install cmake`

#### Install Visual Studio
- Visual Studio 2019 or 2022
- Include "Desktop development with C++"

---

## Building the Project

### Step 1: Get the Source
```bash
# If you have a git repository
git clone https://github.com/yourusername/cueforge-qt.git
cd cueforge-qt

# Or extract from archive
unzip cueforge-qt.zip
cd cueforge-qt
```

### Step 2: Configure Build

#### macOS/Linux
```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Or specify Qt location explicitly
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.0/gcc_64
```

#### Windows (MSVC)
```powershell
# Create build directory
mkdir build
cd build

# Configure (adjust Qt path as needed)
cmake .. -G "Visual Studio 17 2022" ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64
```

#### Windows (MinGW)
```powershell
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\mingw_64
```

### Step 3: Build

#### macOS/Linux
```bash
# Build (parallel jobs for speed)
make -j$(nproc)

# Or use CMake build command
cmake --build . --parallel
```

#### Windows
```powershell
# Build Release configuration
cmake --build . --config Release

# Or open in Visual Studio
start CueForge.sln
```

### Step 4: Run

#### macOS/Linux
```bash
./bin/CueForge
```

#### Windows
```powershell
.\bin\Release\CueForge.exe
```

---

## Troubleshooting

### Qt Not Found

**Error:** `Could not find Qt6`

**Solution:**
```bash
# Set CMAKE_PREFIX_PATH
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.0/[compiler]

# Examples:
# macOS: -DCMAKE_PREFIX_PATH=/usr/local/opt/qt@6
# Linux: -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/qt6
# Windows: -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64
```

### Wrong Qt Version

**Error:** `Qt version X.X.X too old`

**Solution:**
```bash
# Check installed version
qmake --version

# Install Qt 6.5+ from qt.io
# Or update via package manager
brew upgrade qt@6  # macOS
sudo apt-get upgrade qt6-base-dev  # Linux
```

### Compiler Errors

**Error:** `C++17 support required`

**Solution:**
```bash
# GCC: Upgrade to 7+
sudo apt-get install gcc-9 g++-9

# Set compiler explicitly
cmake .. -DCMAKE_CXX_COMPILER=g++-9
```

### Missing Qt Modules

**Error:** `Could not find Qt6Multimedia`

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install qt6-multimedia-dev

# macOS
brew reinstall qt@6  # Includes all modules

# Windows
# Rerun Qt Installer and add Multimedia module
```

### Build Fails on Windows

**Error:** `LINK : fatal error LNK1104: cannot open file 'Qt6Core.lib'`

**Solution:**
1. Ensure Qt path is correct in CMAKE_PREFIX_PATH
2. Check you're building for correct architecture (x64)
3. Rebuild from clean:
```powershell
rmdir /s /q build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

---

## Advanced Options

### Debug Build
```bash
# Configure for debug
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build . --config Debug

# Run with debugger
gdb ./bin/CueForge  # Linux
lldb ./bin/CueForge  # macOS
```

### Build with Tests
```bash
cmake .. -DBUILD_TESTS=ON
cmake --build .
ctest  # Run tests
```

### Custom Install Location
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/cueforge
cmake --build .
sudo cmake --install .
```

### Ninja Generator (Faster Builds)
```bash
# Install ninja
brew install ninja  # macOS
sudo apt-get install ninja-build  # Linux

# Build with ninja
cmake .. -G Ninja
ninja
```

### Build with Clang
```bash
cmake .. \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++
make
```

### Static Build (Single Executable)
```bash
cmake .. -DBUILD_SHARED_LIBS=OFF
cmake --build .
```

---

## Build Output

Successful build creates:
```
build/
└── bin/
    ├── CueForge           # Main executable
    └── [platform-specific files]
```

### File Sizes (Approximate)
- **Debug build**: 50-100 MB
- **Release build**: 10-20 MB
- **With Qt libraries**: 100-200 MB

---

## Next Steps

After successful build:

1. **Run the application**: `./bin/CueForge`
2. **Read the README**: `cat ../README.md`
3. **Explore the code**: Start with `src/main.cpp`
4. **Check documentation**: Browse `docs/` folder

---

## Getting Help

- **Build issues**: Check Troubleshooting section above
- **Qt documentation**: https://doc.qt.io/qt-6/
- **CMake documentation**: https://cmake.org/documentation/
- **GitHub Issues**: [your-repo-url]/issues

---

**Last Updated**: 2025
**Tested Platforms**: macOS 14, Ubuntu 22.04, Windows 11
**Qt Version**: 6.5.0+