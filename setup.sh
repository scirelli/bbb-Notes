#!/bin/bash

bulkStorageDevice=/dev/mmcblk0
bulkStorageMntPnt=/media/sdcard
userHomes="$bulkStorageMntPnt/home"
userName='cck'

# First time
mount "$bulkStorageDevice" "$bulkStorageMntPnt"
mkdir --parents --verbose "$userHomes"
useradd --create-home --shell /bin/bash --home "$userHomes/$userName" --password "$(perl -e "print crypt('temppwd','sa');")" "$userName"
passwd --expire "$userName"
echo alias=\'ls -lah\' > "$userHomes/$userName/.bash_aliases"
