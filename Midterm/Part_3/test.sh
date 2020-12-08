#!/bin/bash

dd if=/dev/urandom of=linuxFile.data bs=1K count=180
make clean
make
./fileSystemOper fileSystem.data mkdir "/usr"
./fileSystemOper fileSystem.data mkdir "/usr/ysa"
./fileSystemOper fileSystem.data mkdir "/bin/ysa"
./fileSystemOper fileSystem.data write "/usr/ysa/file1" linuxFile.data
./fileSystemOper fileSystem.data write "/usr/file2" linuxFile.data
./fileSystemOper fileSystem.data write "/file3" linuxFile.data
./fileSystemOper fileSystem.data list "/"
./fileSystemOper fileSystem.data del "/usr/ysa/file1"
./fileSystemOper fileSystem.data dumpe2fs
./fileSystemOper fileSystem.data read "/usr/file2" linuxFile2.data
md5sum linuxFile2.data linuxFile.data
./fileSystemOper fileSystem.data ln "/usr/file2" "/usr/linkedfile2"
./fileSystemOper fileSystem.data list "/usr"
./fileSystemOper fileSystem.data write "/usr/linkedfile2" linuxFile.data
./fileSystemOper fileSystem.data lnsym "/usr/file2" "/usr/symlinkedfile2"
./fileSystemOper fileSystem.data list "/usr"
./fileSystemOper fileSystem.data del "/usr/file2"
./fileSystemOper fileSystem.data list "/usr"
./fileSystemOper fileSystem.data write "/usr/symlinkedfile2" linuxFile.data
for i in {1..35}; do ./fileSystemOper fileSystem.data mkdir /x$i; done
./fileSystemOper fileSystem.data write "/fragmented" linuxFile.data
./fileSystemOper fileSystem.data dumpe2fs
./fileSystemOper fileSystem.data fsck
