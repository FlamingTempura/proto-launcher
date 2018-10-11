#!/bin/sh

cd "$(dirname "$0")"

node build
sudo rm -r /usr/local/bin/launcher /usr/local/bin/launcher-linux-x64
sudo mv ../bin/* /usr/local/bin

echo "Installation successful!"
echo ""
echo "Start the launcher by running \"launcher\""
echo ""
echo "To significantly reduce loading time, run \"launcher --service\" on login."