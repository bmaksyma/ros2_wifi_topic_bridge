#!/bin/bash

set -e

echo "=== [1/7] Cleaning the system from old libtins versions ==="
sudo apt-get remove -y libtins-dev libtins* || true
sudo rm -f /usr/local/lib/libtins.*
sudo rm -rf /usr/local/include/tins
sudo ldconfig

echo "=== [2/7] Installing dependencies ==="
sudo apt-get update
sudo apt-get install -y libpcap-dev libssl-dev cmake build-essential git

echo "=== [3/7] Preparing working directory ==="
cd ~
rm -rf ~/libtins

echo "=== [4/7] Downloading libtins and PR #538 (Action Frames support) ==="
git clone https://github.com/mfontanini/libtins.git
cd libtins
git fetch origin pull/538/head:pr538
git checkout pr538

echo "=== [5/7] Building the library ==="
mkdir build
cd build
cmake ../ -DLIBTINS_ENABLE_CXX11=1 -DLIBTINS_ENABLE_DOT11=1
make -j$(nproc)

echo "=== [6/7] Installing to the system ==="
sudo make install
sudo ldconfig

echo "=== [7/7] DONE! ==="