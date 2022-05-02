#!/bin/bash

bulkStorageDevice=/dev/mmcblk0
bulkStorageMntPnt=/media/sdcard
userHomes="$bulkStorageMntPnt/home"
userName='cck'

/opt/scripts/tools/update_kernel.sh

# == First time
#Mount sdcard
mount "$bulkStorageDevice" "$bulkStorageMntPnt"
# Auto-mount after boot
#       device-spec         mount-point    fs-type options dump pass
echo "$bulkStorageDevice bulkStorageMntPnt ext4 defaults 0 2" >> /etc/fstab
mkdir --parents --verbose "$userHomes"
useradd --create-home --shell /bin/bash --home "$userHomes/$userName" --password "$(perl -e "print crypt('temppwd','sa');")" "$userName"
passwd --expire "$userName"
echo alias=\'ls -lah\' > "$userHomes/$userName/.bash_aliases"
usermod -a -G kmem,dialout,cdrom,floppy,audio,dip,video,plugdev,bluetooth,netdev,i2c,xenomai,tisdk,docker,iio,spi,remoteproc,eqep,pwm,gpio "$userName"

# == setup swap space
swapon --show
# There should be none on BBB
swapfile=/media/sdcard/swapfile
fallocate -l 1G "$swapfile"
chmod 600 "$swapfile"
mkswap "$swapfile"
swapon "$swapfile"
echo "$swapfile swap swap defaults 0 2" >> /etc/fstab
cat /proc/sys/vm/swappiness
# cat /proc/sys/vm/swappiness
# set swap use to be low in
# sysctl vm.swappiness=10
# persist value in
# vim /etc/sysctl.conf

# == Turn swap off
# swapoff -v "$swapfile"
# rm "$swapfile"
# ## remove it from /etc/fstab


# == pyenv
apt-get update; sudo apt-get install make build-essential libssl-dev zlib1g-dev \
    libbz2-dev libreadline-dev libsqlite3-dev wget curl llvm \
    libncursesw5-dev xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev

sudo -H -u cck bash -c 'curl https://pyenv.run | bash'

homedir=$( getent passwd "$userName" | cut -d: -f6 )
cat << 'EOF' >> "$homedir/.bashrc"
export EDITOR=vim
export PYTHONBREAKPOINT=pudb.set_trace
export PYTHON_VENV_DIR_NAME=.venv
PIPENV_VENV_IN_PROJECT=1

export PATH="$HOME/.pyenv/bin:$PATH"
eval "$(pyenv init --path)"
eval "$(pyenv virtualenv-init -)"
EOF

sudo -H -u cck bash -c 'pyenv install 3.10.4'
sudo -H -u cck bash -c 'pyenv global 3.10.4'

# == WiFi setup
apt install -y linux-headers-4.19.94-ti-r73 dkms bc
git clone https://github.com/brektrou/rtl8821CU.git /tmp/rtl8821CU
cd /tmp/rtl8821CU || exit
./dkms-install.sh
modprobe 8821cu
