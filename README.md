# example
```
// install mysql client
sudo yum install mysql
sudo yum install mysql++-devel.x86_64
sudo yum install -y automake
sudo yum install libuuid-devel.x86_64
sudo pip3 install conan

export CPLUS_INCLUDE_PATH="/usr/include/mysql/:$CPLUS_INCLUDE_PATH"
export LIBRARY_PATH=/usr/lib64/mysql/:$LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/lib64/mysql/:$LD_LIBRARY_PATH

// install cmake
wget https://cmake.org/files/v3.23/cmake-3.23.1.tar.gz
tar -zxvf cmake-3.23.1.tar.gz
cd cmake-3.23.1
./bootstrap --prefix=~
make -j 9
make install

// install xmake
// https://xmake.io/#/zh-cn/guide/installation
git clone --recursive https://github.com/xmake-io/xmake.git
cd ./xmake
git checkout 2d3313ba1a2aa40350be90d52082f2e0f2cfedb4
make build
./scripts/get.sh __local__ __install_only__
source ~/.xmake/profile

git clone https://github.com/fantasy-peak/example.git
cd example && mkdir build && cd build
cmake ..
make -j8
```