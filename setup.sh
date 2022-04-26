#!/bin/bash

bulkStorageDevice=/dev/mmcblk0
bulkStorageMntPnt=/media/sdcard
userHomes="$bulkStorageMntPnt/home"
userName='cck'

/opt/scripts/tools/update_kernel.sh

# First time
mount "$bulkStorageDevice" "$bulkStorageMntPnt"
mkdir --parents --verbose "$userHomes"
useradd --create-home --shell /bin/bash --home "$userHomes/$userName" --password "$(perl -e "print crypt('temppwd','sa');")" "$userName"
passwd --expire "$userName"
echo alias=\'ls -lah\' > "$userHomes/$userName/.bash_aliases"

#pyenv
apt-get update; sudo apt-get install make build-essential libssl-dev zlib1g-dev \
    libbz2-dev libreadline-dev libsqlite3-dev wget curl llvm \
    libncursesw5-dev xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev

sudo -H -u cck bash -c 'curl https://pyenv.run | bash'

homedir=$( getent passwd "$userName" | cut -d: -f6 )
cat << 'EOF' >> "$homedir/.bashrc"
export EDITOR=vim
export PYTHONBREAKPOINT=pudb.set_trace
export PYTHON_VENV_DIR_NAME=.venv
export PATH="$HOME/.pyenv/bin:$PATH"
eval "$(pyenv init --path)"
eval "$(pyenv virtualenv-init -)"
EOF

sudo -H -u cck bash -c 'pyenv install 3.10.4'
sudo -H -u cck bash -c 'pyenv global 3.10.4'
