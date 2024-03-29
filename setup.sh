#!/bin/bash
# Execute as sudo

set -e
set -o xtrace
set -o pipefail

bulkStorageDevice=/dev/mmcblk0
bulkStorageMntPnt=/media/sdcard
userHomes="$bulkStorageMntPnt/home"
userName='cck'

if [ ! -f /home/debian/.setup_kernal1 ]; then
    echo 'Updating Kernel.'
    cd /opt/scripts
    sudo -H -u debian bash -c 'git pull'
    /opt/scripts/tools/update_kernel.sh
    /opt/scripts/tools/version.sh
    touch /home/debian/.setup_kernal1
    echo 'Run this script again after reboot.'
    shutdown -r 1
fi

if [ ! -f /home/debian/.setup_kernal2 ]; then
    # Turn off HDMI and audio out to enable more GPIO pins
    echo Turn off HDMI and audio out to enable more GPIO pins
    sed -i s/#disable_uboot_overlay_video=1/disable_uboot_overlay_video=1/g /boot/uEnv.txt
    sed -i s/#disable_uboot_overlay_audio=1/disable_uboot_overlay_audio=1/g /boot/uEnv.txt
    touch /home/debian/.setup_kernal2
    echo 'Run this script again after reboot.'
    shutdown -r 1
fi

echo 'configure P9_31 as PRU output'
# This is okay to run multiple times.
config-pin P9_31 pruout

apt update
apt upgrade -y

# == First time
if [ -b "$bulkStorageDevice" ]; then
    # Format the sdcard for ext4 (this will erase the sdcard.
    mkfs -t ext4 "$bulkStorageDevice"

    #Mount sdcard
    mkdir --parents "$bulkStorageMntPnt"
    mount "$bulkStorageDevice" "$bulkStorageMntPnt"
    mkdir --parents "$bulkStorageMntPnt/tmp"
    chmod 777 "$bulkStorageMntPnt/tmp"
    chmod +t "$bulkStorageMntPnt/tmp"
    mount --bind "$bulkStorageMntPnt/tmp" /tmp
    findmnt /tmp
    # Auto-mount after boot
    #       device-spec         mount-point    fs-type options dump pass
    echo "$bulkStorageDevice $bulkStorageMntPnt ext4 noatime,defaults 0 2" >> /etc/fstab
    echo "/media/sdcard/tmp /tmp none noatime,defaults,bind 0 2" >> /etc/fstab
    mkdir --parents --verbose "$userHomes"
    useradd --create-home --shell /bin/bash --home "$userHomes/$userName" --password "$(perl -e "print crypt('temppwd','sa');")" "$userName"
    passwd --expire "$userName"
    echo alias ll=\'ls -lah\' > "$userHomes/$userName/.bash_aliases"
    chown "$userName:$userName" "$userHomes/$userName/.bash_aliases"
    usermod -a -G kmem,dialout,cdrom,floppy,audio,dip,video,plugdev,bluetooth,netdev,i2c,xenomai,tisdk,docker,iio,spi,remoteproc,eqep,pwm,gpio "$userName"
    ln -s "/media/sdcard/home/$userName" "/home/$userName"
    chown -h "$userName":"$userName" "/home/$userName"
    mkdir -p "/home/$userName/Projects"
    chown "$userName":"$userName" "/home/$userName/Projects"

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
fi



# == pyenv
apt-get update; apt-get install -y make build-essential libssl-dev zlib1g-dev \
    libbz2-dev libreadline-dev libsqlite3-dev wget curl llvm \
    libncursesw5-dev xz-utils tk-dev libxml2-dev libxmlsec1-dev libffi-dev liblzma-dev

sudo -Hi -u cck bash -c 'curl https://pyenv.run | bash'

homedir=$( getent passwd "$userName" | cut -d: -f6 )
cat << 'EOF' >> "$homedir/.bashrc"
force_color_prompt=yes

export EDITOR=vim
export PYTHONBREAKPOINT=pudb.set_trace
export PYTHON_VENV_DIR_NAME=.venv
PIPENV_VENV_IN_PROJECT=1

export PATH="$HOME/.pyenv/bin:$PATH"
eval "$(pyenv init --path)"
eval "$(pyenv virtualenv-init -)"
EOF

#runuser -l cck -c -- env PYTHON_CONFIGURE_OPTS="--enable-shared CC=clang" pyenv install 3.10.4
#runuser -l cck -c -- pyenv global 3.10.4
pyenv="$userHomes/$userName/.pyenv/bin/pyenv"
sudo -Hi -u cck bash -c 'env PYTHON_CONFIGURE_OPTS="--enable-shared" "'$pyenv'" install 3.10.4 && "'$pyenv'" global 3.10.4'

## == WiFi setup
# apt install -y linux-headers-4.19.94-ti-r73 dkms bc
# git clone https://github.com/brektrou/rtl8821CU.git /tmp/rtl8821CU
# cd /tmp/rtl8821CU || exit
# ./dkms-install.sh
# modprobe 8821cu

# Install and update some packages
apt install -y build-essential gdb strace network-manager vim git wpasupplicant sudo systemd screen gzip ca-certificates bluetooth apt xxd rsyslog openssl udev locales file git-man connman dbus distro-info-data clang libtool-bin libpython3-dev

sudo -Hi -u cck bash -c 'curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.1/install.sh | bash'
#sudo -Hi -u cck bash -c 'nvm install node'  # "node" is an alias for the latest version

# File sharing if you're not comfortable working through ssh and vim
apt install -y samba

mv /etc/samba/smb.conf /etc/samba/smb.conf.bak
cat <<EOF > /etc/samba/smb.conf
#======================= Global Settings =======================
[global]
security = user
allow insecure wide links = yes
   workgroup = WORKGROUP
   log file = /var/log/samba/log.%m
   max log size = 1000
   logging = file
   panic action = /usr/share/samba/panic-action %d
   server role = standalone server
   obey pam restrictions = yes
   unix password sync = yes
   passwd program = /usr/bin/passwd %u
   passwd chat = *Enter\snew\s*\spassword:* %n\n *Retype\snew\s*\spassword:* %n\n *password\supdated\ssuccessfully* .
   pam password change = yes
   map to guest = bad user
   usershare allow guests = yes

#======================= Share Definitions =======================
[homes]
   comment = Home Directories
   browseable = no
   read only = no
   create mask = 0700
   directory mask = 0700
   valid users = %S

[sambashare]
    comment = Samba on Debian
    path = $userHomes/$userName/sambashare
    read only = no
    browsable = yes
    writeable = yes
    follow symlinks = yes
    wide links = yes
    valid users = cck
    create mask = 0644
    directory mask = 0755
EOF
service smbd restart
echo "$userName" SMB account
smbpasswd -a $userName

# The following network interfaces were found in /etc/network/interfaces
# which means they are currently configured by ifupdown:
# - default
# - wlan0
# If you want to manage those interfaces with NetworkManager instead
# remove their configuration from /etc/network/interfaces.


# https://developpaper.com/building-vim-from-source-code/
git clone https://github.com/vim/vim.git /tmp/vim
cd /tmp/vim || exit 1
./configure \
        --with-features=huge \
        --enable-multibyte \
        --enable-rubyinterp \
        --enable-perlinterp \
        --enable-luainterp \
        --enable-pythoninterp \
        --with-python-config-dir=/usr/lib/python2.7/config-arm-linux-gnueabihf/ \
        --enable-python3interp \
        --with-python3-config-dir=/media/sdcard/home/cck/.pyenv/versions/3.10.4/lib/python3.10/config-3.10-arm-linux-gnueabihf \
        --enable-cscope \
        --prefix=/usr

make && make install

# Updates based on BBB docs
sudo apt install -y ti-tidl mjpg-streamer-opencv-python
cd /var/lib/cloud9
sudo -H -u debian bash -c 'git pull'

rm -f /home/debian/.setup_kernal1
rm -f /home/debian/.setup_kernal2
