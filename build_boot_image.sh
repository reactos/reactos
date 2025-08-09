#!/bin/bash
# ReactOS Two-Stage FreeLoader Boot Image Builder
# This script creates bootable disk images with freeldr.sys and rosload.exe
# Place this script in the ReactOS source root directory

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR"
BUILD_DIR="${BUILD_DIR:-$SOURCE_DIR/output-MinGW-amd64}"
OUTPUT_DIR="$SOURCE_DIR/boot_images"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

# Print header
print_header() {
    echo
    echo "============================================"
    echo "  ReactOS Two-Stage Boot Image Builder"
    echo "============================================"
    echo "  Source: $SOURCE_DIR"
    echo "  Build:  $BUILD_DIR"
    echo "  Output: $OUTPUT_DIR"
    echo "============================================"
    echo
}

# Check prerequisites
check_requirements() {
    log_step "Checking requirements..."
    
    local missing=0
    
    # Check for required tools
    for cmd in dd mkfs.fat mtools sfdisk mkisofs qemu-system-x86_64; do
        if ! command -v $cmd &> /dev/null; then
            log_error "$cmd is not installed"
            missing=1
        else
            log_info "Found $cmd"
        fi
    done
    
    # Check for built binaries
    if [ ! -f "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" ]; then
        log_error "freeldr.sys not found in $BUILD_DIR"
        log_warn "Please build ReactOS first: cd $SOURCE_DIR && ./configure.sh && cd output-MinGW-amd64 && ninja"
        missing=1
    else
        log_info "Found freeldr.sys ($(stat -c%s $BUILD_DIR/boot/freeldr/freeldr/freeldr.sys) bytes)"
    fi
    
    if [ ! -f "$BUILD_DIR/boot/freeldr/freeldr/rosload.exe" ]; then
        log_error "rosload.exe not found in $BUILD_DIR"
        missing=1
    else
        log_info "Found rosload.exe ($(stat -c%s $BUILD_DIR/boot/freeldr/freeldr/rosload.exe) bytes)"
    fi
    
    if [ ! -f "$BUILD_DIR/boot/freeldr/freeldr/freeldr_pe.exe" ]; then
        log_warn "freeldr_pe.exe not found (optional)"
    else
        log_info "Found freeldr_pe.exe ($(stat -c%s $BUILD_DIR/boot/freeldr/freeldr/freeldr_pe.exe) bytes)"
    fi
    
    if [ $missing -eq 1 ]; then
        log_error "Missing requirements. Please install missing tools or build ReactOS."
        exit 1
    fi
    
    log_info "All requirements satisfied"
}

# Create output directories
create_directories() {
    log_step "Creating output directories..."
    
    rm -rf "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"/{images,temp,logs}
    
    log_info "Created directory structure"
}

# Create FreeLdr configuration
create_freeldr_config() {
    log_step "Creating FreeLdr configuration..."
    
    cat > "$OUTPUT_DIR/temp/freeldr.ini" << 'EOF'
[FREELOADER]
DefaultOS=ReactOS
TimeOut=10
ShowTime=Yes
MenuColor=Blue
MenuTextColor=Yellow
SelectedColor=Black
SelectedTextColor=Green
BackdropColor=Blue
BackdropTextColor=White
TitleText=ReactOS Two-Stage Boot Test

[ReactOS]
BootType=ReactOSSetup
SystemPath=\
Options=/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /BOOTLOG

[ReactOS_Screen]
BootType=ReactOSSetup  
SystemPath=\
Options=/DEBUG /DEBUGPORT=SCREEN /SOS

[Safe]
BootType=ReactOSSetup
SystemPath=\
Options=/SAFEBOOT:MINIMAL /SOS

[Operating Systems]
ReactOS="ReactOS"
ReactOS_Screen="ReactOS (Debug to Screen)"
Safe="ReactOS (Safe Mode)"
EOF
    
    log_info "Created freeldr.ini configuration"
}

# Build Type 1: Simple raw image
build_raw_image() {
    log_step "Building raw boot image..."
    
    local IMAGE="$OUTPUT_DIR/images/raw_boot.img"
    
    # Copy freeldr.sys as base
    cp "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" "$IMAGE"
    
    # Pad to 4MB
    dd if=/dev/zero of="$IMAGE" bs=1M count=4 seek=1 status=none 2>/dev/null
    
    log_info "Created raw_boot.img ($(stat -c%s $IMAGE) bytes)"
}

# Build Type 2: Concatenated image with rosload at fixed offset
build_concat_image() {
    log_step "Building concatenated boot image..."
    
    local IMAGE="$OUTPUT_DIR/images/concat_boot.img"
    
    # Start with freeldr.sys
    cp "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" "$IMAGE"
    
    # Pad to 1MB
    dd if=/dev/zero of="$IMAGE" bs=1M count=1 seek=1 status=none 2>/dev/null
    
    # Place rosload.exe at 1MB offset
    dd if="$BUILD_DIR/boot/freeldr/freeldr/rosload.exe" of="$IMAGE" bs=1M seek=1 conv=notrunc status=none
    
    # Pad to 8MB total
    dd if=/dev/zero of="$IMAGE" bs=1M count=8 seek=2 status=none 2>/dev/null
    
    log_info "Created concat_boot.img with rosload at 1MB offset"
}

# Build Type 3: FAT16 disk image with MBR
build_fat16_image() {
    log_step "Building FAT16 disk image..."
    
    local IMAGE="$OUTPUT_DIR/images/fat16_boot.img"
    local SIZE_MB=32
    
    # Create blank disk
    dd if=/dev/zero of="$IMAGE" bs=1M count=$SIZE_MB status=none
    
    # Create MBR partition table
    cat << EOF | sfdisk "$IMAGE" --no-reread >/dev/null 2>&1
label: dos
label-id: 0xDEADBEEF
device: $IMAGE
unit: sectors

${IMAGE}1 : start=2048, size=$((SIZE_MB*2048-2048)), type=6
EOF
    
    # Format partition as FAT16
    mkfs.fat -F 16 -n "REACTOS" -R 8 --offset 2048 "$IMAGE" $((SIZE_MB*2048-2048)) >/dev/null 2>&1
    
    # Copy files using mtools with offset
    export MTOOLS_SKIP_CHECK=1
    
    # Copy bootloader files
    mcopy -i "$IMAGE@@1M" "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" ::FREELDR.SYS
    mcopy -i "$IMAGE@@1M" "$BUILD_DIR/boot/freeldr/freeldr/rosload.exe" ::ROSLOAD.EXE
    mcopy -i "$IMAGE@@1M" "$OUTPUT_DIR/temp/freeldr.ini" ::FREELDR.INI
    
    # Create loader directory and copy rosload there too
    mmd -i "$IMAGE@@1M" ::LOADER 2>/dev/null || true
    mcopy -i "$IMAGE@@1M" "$BUILD_DIR/boot/freeldr/freeldr/rosload.exe" ::LOADER/ROSLOAD.EXE
    
    # Install MBR boot code (preserve partition table)
    dd if="$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" of="$IMAGE" bs=1 count=440 conv=notrunc status=none
    
    # List contents
    log_info "FAT16 contents:"
    mdir -i "$IMAGE@@1M" :: 2>/dev/null || true
    
    log_info "Created fat16_boot.img"
}

# Build Type 4: FAT32 disk image
build_fat32_image() {
    log_step "Building FAT32 disk image..."
    
    local IMAGE="$OUTPUT_DIR/images/fat32_boot.img"
    local SIZE_MB=64
    
    # Create blank disk
    dd if=/dev/zero of="$IMAGE" bs=1M count=$SIZE_MB status=none
    
    # Create MBR partition table
    cat << EOF | sfdisk "$IMAGE" --no-reread >/dev/null 2>&1
label: dos
device: $IMAGE
unit: sectors

${IMAGE}1 : start=2048, type=c
EOF
    
    # Format as FAT32
    mkfs.fat -F 32 -n "REACTOS32" --offset 2048 "$IMAGE" $((SIZE_MB*2048-2048)) >/dev/null 2>&1
    
    # Copy files
    export MTOOLS_SKIP_CHECK=1
    mcopy -i "$IMAGE@@1M" "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" ::freeldr.sys
    mcopy -i "$IMAGE@@1M" "$BUILD_DIR/boot/freeldr/freeldr/rosload.exe" ::rosload.exe
    mcopy -i "$IMAGE@@1M" "$OUTPUT_DIR/temp/freeldr.ini" ::freeldr.ini
    mmd -i "$IMAGE@@1M" ::loader 2>/dev/null || true
    mcopy -i "$IMAGE@@1M" "$BUILD_DIR/boot/freeldr/freeldr/rosload.exe" ::loader/rosload.exe
    
    # Install MBR
    dd if="$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" of="$IMAGE" bs=1 count=440 conv=notrunc status=none
    
    log_info "Created fat32_boot.img"
}

# Build Type 5: ISO image
build_iso_image() {
    log_step "Building ISO image..."
    
    local IMAGE="$OUTPUT_DIR/images/boot.iso"
    local ISO_DIR="$OUTPUT_DIR/temp/iso"
    
    # Create ISO directory structure
    mkdir -p "$ISO_DIR"/{loader,boot}
    
    # Copy files
    cp "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" "$ISO_DIR/"
    cp "$BUILD_DIR/boot/freeldr/freeldr/rosload.exe" "$ISO_DIR/"
    cp "$BUILD_DIR/boot/freeldr/freeldr/rosload.exe" "$ISO_DIR/loader/"
    cp "$OUTPUT_DIR/temp/freeldr.ini" "$ISO_DIR/"
    
    # Copy boot sector if available
    if [ -f "$BUILD_DIR/boot/freeldr/bootsect/isoboot.bin" ]; then
        cp "$BUILD_DIR/boot/freeldr/bootsect/isoboot.bin" "$ISO_DIR/boot/"
        
        # Create bootable ISO
        mkisofs -R -J -l -no-emul-boot -boot-load-size 4 -boot-info-table \
            -b boot/isoboot.bin -o "$IMAGE" "$ISO_DIR" >/dev/null 2>&1
    else
        # Create non-bootable ISO
        mkisofs -R -J -l -o "$IMAGE" "$ISO_DIR" >/dev/null 2>&1
        log_warn "isoboot.bin not found, ISO may not be bootable"
    fi
    
    log_info "Created boot.iso"
}

# Test boot image
test_boot_image() {
    local IMAGE="$1"
    local NAME="$2"
    local TIMEOUT="${3:-5}"
    
    log_info "Testing $NAME (timeout: ${TIMEOUT}s)..."
    
    local LOG_FILE="$OUTPUT_DIR/logs/$(basename $IMAGE .img).log"
    
    timeout $TIMEOUT qemu-system-x86_64 \
        -drive file="$IMAGE",format=raw \
        -m 256 \
        -serial stdio \
        -display none \
        -no-reboot \
        2>&1 | tee "$LOG_FILE" || true
    
    # Analyze results
    echo -n "  Results: "
    if grep -q "BootMain" "$LOG_FILE" 2>/dev/null; then
        echo -n "✓BootMain "
    else
        echo -n "✗BootMain "
    fi
    
    if grep -q "Memory" "$LOG_FILE" 2>/dev/null; then
        echo -n "✓ Memory "
    else
        echo -n "✗ Memory "
    fi
    
    if grep -q "rosload" "$LOG_FILE" 2>/dev/null; then
        echo -n "✓ Rosload"
    else
        echo -n "✗ Rosload"
    fi
    echo
}

# Show summary
show_summary() {
    echo
    echo "============================================"
    echo "  Build Complete"
    echo "============================================"
    echo
    echo "Images created in: $OUTPUT_DIR/images/"
    echo
    ls -lh "$OUTPUT_DIR/images/" 2>/dev/null
    echo
    echo "To test an image:"
    echo "  qemu-system-x86_64 -drive file=<image>,format=raw -m 256"
    echo
    echo "To test with serial output:"
    echo "  qemu-system-x86_64 -drive file=<image>,format=raw -m 256 -serial stdio -display none"
    echo
    echo "To test with VGA display:"
    echo "  qemu-system-x86_64 -drive file=<image>,format=raw -m 256 -serial stdio"
    echo
}

# Main execution
main() {
    print_header
    check_requirements
    create_directories
    create_freeldr_config
    
    # Build all image types
    build_raw_image
    build_concat_image
    build_fat16_image
    build_fat32_image
    build_iso_image
    
    # Optional testing
    echo
    read -p "Do you want to test the boot images? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        test_boot_image "$OUTPUT_DIR/images/raw_boot.img" "Raw Image" 3
        test_boot_image "$OUTPUT_DIR/images/concat_boot.img" "Concatenated Image" 3
        test_boot_image "$OUTPUT_DIR/images/fat16_boot.img" "FAT16 Image" 3
    fi
    
    show_summary
}

# Run main function
main "$@"