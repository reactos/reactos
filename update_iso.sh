#!/bin/bash

# Script to update ReactOS ISO with new freeldr
set -e

echo "Updating ReactOS ISO with new countdown-enabled freeldr..."

# Create work directory
rm -rf iso_work
mkdir -p iso_work

# Extract ISO
echo "Extracting ISO..."
xorriso -osirrox on -indev livecd.iso -extract / iso_work/ 2>/dev/null || true

# Find and update freeldr files
echo "Updating freeldr files..."
if [ -f "boot/freeldr/freeldr/uefildr.efi" ]; then
    find iso_work -name "uefildr.efi" -exec cp -v boot/freeldr/freeldr/uefildr.efi {} \;
    find iso_work -name "bootx64.efi" -exec cp -v boot/freeldr/freeldr/uefildr.efi {} \;
fi

if [ -f "boot/freeldr/freeldr/freeldr.sys" ]; then
    find iso_work -name "freeldr.sys" -exec cp -v boot/freeldr/freeldr/freeldr.sys {} \;
fi

# Rebuild ISO
echo "Creating new ISO..."
xorriso -as mkisofs \
    -R -J -l \
    -b loader/isoboot.bin \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -eltorito-alt-boot \
    -eltorito-platform efi \
    -b efi/boot/bootx64.efi \
    -no-emul-boot \
    -o livecd_updated.iso \
    iso_work 2>/dev/null || \
xorriso -as mkisofs \
    -R -J -l \
    -b boot/isolinux/isolinux.bin \
    -c boot/isolinux/boot.cat \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -o livecd_updated.iso \
    iso_work 2>/dev/null || \
xorriso -as mkisofs \
    -R -J -l \
    -o livecd_updated.iso \
    iso_work

echo "Done! New ISO: livecd_updated.iso"
echo "Test with: qemu-system-x86_64 -cdrom livecd_updated.iso -m 256 -serial stdio -bios /usr/share/ovmf/OVMF.fd"