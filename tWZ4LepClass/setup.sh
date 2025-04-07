cd /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/tWZ4LepClass/build
cmake -DCMAKE_PREFIX_PATH=/afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/install -DCMAKE_INSTALL_PREFIX=/afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/tWZ4LepClass/install ../
make
make install
source setup.sh
cd /afs/cern.ch/user/k/kebarend/public/tWZ/FastFramesPackage/