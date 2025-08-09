#!/bin/bash
# ReactOS FreeLoader-Only Boot Image Builder
# This script creates a minimal bootable disk image with only freeldr.sys

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR"
BUILD_DIR="${BUILD_DIR:-$SOURCE_DIR/output-MinGW-amd64}"
OUTPUT_DIR="$SOURCE_DIR/freeloader_boot"

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
    echo "  ReactOS FreeLoader-Only Image Builder"
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
    for cmd in dd mkfs.fat mtools sfdisk qemu-system-x86_64; do
        if ! command -v $cmd &> /dev/null; then
            log_error "$cmd is not installed"
            missing=1
        else
            log_info "Found $cmd"
        fi
    done
    
    # Check for freeldr.sys
    if [ ! -f "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" ]; then
        log_error "freeldr.sys not found in $BUILD_DIR"
        log_warn "Please build ReactOS first: cd $SOURCE_DIR && ./configure.sh && cd output-MinGW-amd64 && ninja freeldr"
        missing=1
    else
        local SIZE=$(stat -c%s "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys")
        log_info "Found freeldr.sys ($SIZE bytes)"
    fi
    
    if [ $missing -eq 1 ]; then
        log_error "Missing requirements. Please install missing tools or build FreeLoader."
        exit 1
    fi
    
    log_info "All requirements satisfied"
}

# Create output directories
create_directories() {
    log_step "Creating output directories..."
    
    rm -rf "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"/{images,logs}
    
    log_info "Created directory structure"
}

# Build Type 1: Raw FreeLoader image (simplest)
build_raw_freeloader() {
    log_step "Building raw FreeLoader image..."
    
    local IMAGE="$OUTPUT_DIR/images/freeloader_raw.img"
    
    # Copy freeldr.sys directly as the boot image
    cp "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" "$IMAGE"
    
    # Pad to 1MB for consistent size
    dd if=/dev/zero of="$IMAGE" bs=1M count=1 seek=1 status=none 2>/dev/null
    
    log_info "Created freeloader_raw.img ($(stat -c%s $IMAGE) bytes)"
}

# Build Type 2: Floppy disk image with FreeLoader
build_floppy_image() {
    log_step "Building 1.44MB floppy image with FreeLoader..."
    
    local IMAGE="$OUTPUT_DIR/images/freeloader_floppy.img"
    
    # Create blank 1.44MB floppy image
    dd if=/dev/zero of="$IMAGE" bs=512 count=2880 status=none
    
    # Format as FAT12
    mkfs.fat -F 12 -n "FREELDR" "$IMAGE" >/dev/null 2>&1
    
    # Write FreeLoader boot sector to first 512 bytes
    dd if="$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" of="$IMAGE" bs=512 count=1 conv=notrunc status=none
    
    # Copy FreeLoader to the FAT12 filesystem
    export MTOOLS_SKIP_CHECK=1
    mcopy -i "$IMAGE" "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" ::FREELDR.SYS
    
    log_info "Created freeloader_floppy.img (1.44MB floppy)"
}

# Build Type 3: Small hard disk image with FreeLoader
build_hdd_image() {
    log_step "Building small HDD image with FreeLoader..."
    
    local IMAGE="$OUTPUT_DIR/images/freeloader_hdd.img"
    local SIZE_MB=8
    
    # Create blank disk
    dd if=/dev/zero of="$IMAGE" bs=1M count=$SIZE_MB status=none
    
    # Create MBR partition table with single active partition
    cat << EOF | sfdisk "$IMAGE" --no-reread >/dev/null 2>&1
label: dos
label-id: 0x12345678
device: $IMAGE
unit: sectors

${IMAGE}1 : start=2048, size=$((SIZE_MB*2048-2048)), type=6, bootable
EOF
    
    # Format partition as FAT16
    mkfs.fat -F 16 -n "FREELDR" -R 8 --offset 2048 "$IMAGE" $((SIZE_MB*2048-2048)) >/dev/null 2>&1
    
    # Install MBR boot code from freeldr.sys (first 440 bytes)
    dd if="$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" of="$IMAGE" bs=1 count=440 conv=notrunc status=none
    
    # Copy FreeLoader to the FAT16 partition
    export MTOOLS_SKIP_CHECK=1
    mcopy -i "$IMAGE@@1M" "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" ::FREELDR.SYS
    
    # List contents
    log_info "HDD image contents:"
    mdir -i "$IMAGE@@1M" :: 2>/dev/null || true
    
    log_info "Created freeloader_hdd.img (${SIZE_MB}MB HDD)"
}

# Build Type 4: Minimal bootable CD image
build_iso_image() {
    log_step "Building minimal ISO with FreeLoader..."
    
    local IMAGE="$OUTPUT_DIR/images/freeloader.iso"
    local ISO_DIR="$OUTPUT_DIR/temp_iso"
    
    # Create ISO directory structure
    mkdir -p "$ISO_DIR"
    
    # Copy FreeLoader
    cp "$BUILD_DIR/boot/freeldr/freeldr/freeldr.sys" "$ISO_DIR/FREELDR.SYS"
    
    # Check for isoboot.bin
    if [ -f "$BUILD_DIR/boot/freeldr/bootsect/isoboot.bin" ]; then
        mkdir -p "$ISO_DIR/boot"
        cp "$BUILD_DIR/boot/freeldr/bootsect/isoboot.bin" "$ISO_DIR/boot/"
        
        # Create bootable ISO using El Torito
        mkisofs -R -J -l -no-emul-boot -boot-load-size 4 -boot-info-table \
            -b boot/isoboot.bin -o "$IMAGE" "$ISO_DIR" >/dev/null 2>&1
        
        log_info "Created bootable freeloader.iso"
    else
        # Create non-bootable ISO
        mkisofs -R -J -l -o "$IMAGE" "$ISO_DIR" >/dev/null 2>&1
        log_warn "Created non-bootable ISO (isoboot.bin not found)"
    fi
    
    # Clean up
    rm -rf "$ISO_DIR"
}

# Test boot image with QEMU
test_boot_image() {
    local IMAGE="$1"
    local NAME="$2"
    local TIMEOUT="${3:-10}"
    
    log_info "Testing $NAME (timeout: ${TIMEOUT}s)..."
    
    local LOG_FILE="$OUTPUT_DIR/logs/$(basename $IMAGE).log"
    
    # Run QEMU with timeout
    timeout $TIMEOUT qemu-system-x86_64 \
        -drive file="$IMAGE",format=raw \
        -m 256 \
        -serial stdio \
        -display none \
        -no-reboot \
        -cpu max \
        2>&1 | tee "$LOG_FILE" || true
    
    # Analyze boot log
    echo -n "  Boot stages detected: "
    
    if grep -q "Starting FreeLoader" "$LOG_FILE" 2>/dev/null; then
        echo -n "✓ Starting "
    else
        echo -n "✗ Starting "
    fi
    
    if grep -q "Long mode" "$LOG_FILE" 2>/dev/null; then
        echo -n "✓ LongMode "
    else
        echo -n "✗ LongMode "
    fi
    
    if grep -q "BootMain" "$LOG_FILE" 2>/dev/null; then
        echo -n "✓ BootMain "
    else
        echo -n "✗ BootMain "
    fi
    
    if grep -q "memory" "$LOG_FILE" 2>/dev/null; then
        echo -n "✓ Memory"
    else
        echo -n "✗ Memory"
    fi
    
    echo
    
    # Check for errors
    if grep -q "ASSERT" "$LOG_FILE" 2>/dev/null; then
        log_warn "  Assertion failure detected!"
    fi
    
    if grep -q "not supported" "$LOG_FILE" 2>/dev/null; then
        log_warn "  CPU compatibility issue detected!"
    fi
}

# Interactive testing menu
interactive_test() {
    echo
    echo "Select image to test:"
    echo "1) Raw FreeLoader image"
    echo "2) Floppy image (1.44MB)"
    echo "3) HDD image (8MB)"
    echo "4) ISO image"
    echo "5) Test all"
    echo "0) Skip testing"
    
    read -p "Choice: " choice
    
    case $choice in
        1)
            test_boot_image "$OUTPUT_DIR/images/freeloader_raw.img" "Raw FreeLoader" 10
            ;;
        2)
            test_boot_image "$OUTPUT_DIR/images/freeloader_floppy.img" "Floppy Image" 10
            ;;
        3)
            test_boot_image "$OUTPUT_DIR/images/freeloader_hdd.img" "HDD Image" 10
            ;;
        4)
            test_boot_image "$OUTPUT_DIR/images/freeloader.iso" "ISO Image" 10
            ;;
        5)
            test_boot_image "$OUTPUT_DIR/images/freeloader_raw.img" "Raw FreeLoader" 5
            test_boot_image "$OUTPUT_DIR/images/freeloader_floppy.img" "Floppy Image" 5
            test_boot_image "$OUTPUT_DIR/images/freeloader_hdd.img" "HDD Image" 5
            test_boot_image "$OUTPUT_DIR/images/freeloader.iso" "ISO Image" 5
            ;;
        *)
            log_info "Skipping tests"
            ;;
    esac
}

# Show summary
show_summary() {
    echo
    echo "============================================"
    echo "  FreeLoader-Only Images Created"
    echo "============================================"
    echo
    echo "Images created in: $OUTPUT_DIR/images/"
    echo
    ls -lh "$OUTPUT_DIR/images/" 2>/dev/null
    echo
    echo "Quick test commands:"
    echo
    echo "  # Test raw image:"
    echo "  qemu-system-x86_64 -drive file=$OUTPUT_DIR/images/freeloader_raw.img,format=raw -m 256"
    echo
    echo "  # Test with serial output:"
    echo "  qemu-system-x86_64 -drive file=$OUTPUT_DIR/images/freeloader_hdd.img,format=raw -m 256 -serial stdio -display none"
    echo
    echo "  # Test floppy:"
    echo "  qemu-system-x86_64 -fda $OUTPUT_DIR/images/freeloader_floppy.img -m 256"
    echo
    echo "  # Test ISO:"
    echo "  qemu-system-x86_64 -cdrom $OUTPUT_DIR/images/freeloader.iso -m 256"
    echo
    echo "Debug tips:"
    echo "  - Add '-d int' to see interrupts"
    echo "  - Add '-d cpu_reset' to see triple faults"
    echo "  - Add '-monitor stdio' for QEMU monitor"
    echo
}

# Main execution
main() {
    print_header
    check_requirements
    create_directories
    
    # Build all image types
    build_raw_freeloader
    build_floppy_image
    build_hdd_image
    build_iso_image
    
    # Interactive testing
    interactive_test
    
    show_summary
}

# Run main function
main "$@"