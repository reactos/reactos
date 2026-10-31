#include "recovery_core.h"

#include <string.h>

typedef struct _RECOVERY_EXTENT {
    uint32_t Lbn;
    uint32_t Blocks;
    uint32_t Bytes;
    uint16_t Ref;
} RECOVERY_EXTENT;

/* Directory data is read through its extent list. Directories that grew
 * in many small steps can have far more extents than the metadata files,
 * so the scratch list holds as many as a 16 MB directory can use. */
#define UDFS_RECOVERY_MAX_DIR_EXTENTS \
    ((16U * 1024 * 1024) / UDFS_RECOVERY_SECTOR_SIZE)

typedef struct _RECOVERY_EXTENT_LIST {
    RECOVERY_EXTENT *Items;
    uint32_t Capacity;
    uint32_t Count;
    uint32_t TotalBlocks;
    uint64_t FileBytes;
    uint32_t InlineFeLbn;
    uint32_t InlineOffset;
    uint8_t Inline;
} RECOVERY_EXTENT_LIST;

/* Empty the list but keep its storage. */
static void
ResetExtents(RECOVERY_EXTENT_LIST *extents)
{
    RECOVERY_EXTENT *items = extents->Items;
    uint32_t capacity = extents->Capacity;
    memset(extents, 0, sizeof(*extents));
    extents->Items = items;
    extents->Capacity = capacity;
}

typedef struct _RECOVERY_SECTOR_CACHE {
    uint32_t Lba;
    uint8_t Data[UDFS_RECOVERY_SECTOR_SIZE];
    int Valid;
} RECOVERY_SECTOR_CACHE;

static uint16_t
Read16(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t
Read32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t
Read64(const uint8_t *p)
{
    return (uint64_t)Read32(p) | ((uint64_t)Read32(p + 4) << 32);
}

static int
RecoveryDifference(const void *left, const void *right, size_t length)
{
    const uint8_t *a = (const uint8_t *)left;
    const uint8_t *b = (const uint8_t *)right;
    uint8_t difference = 0;
    size_t i;
    for (i = 0; i < length; ++i)
        difference |= a[i] ^ b[i];
    return difference;
}

static void
Write16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void
Write32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void
Write64(uint8_t *p, uint64_t value)
{
    Write32(p, (uint32_t)value);
    Write32(p + 4, (uint32_t)(value >> 32));
}

static uint16_t
TagCrc(const uint8_t *p, uint32_t length)
{
    uint16_t crc = 0;
    uint32_t i;
    for (i = 0; i < length; ++i) {
        uint32_t bit;
        crc ^= (uint16_t)p[i] << 8;
        for (bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    return crc;
}

#define UDFS_JOURNAL_CRC_LENGTH 150
#define UDFS_JOURNAL_IMP_LENGTH 70
#define UDFS_LEGACY_LVID_CRC_LENGTH 126

static const uint8_t UdfsJournalMagic[8] = {
    'R', 'O', 'S', 'L', 'V', 'I', 'D', '1'
};

static void
RetagLvid(uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE], uint32_t lba)
{
    uint32_t checksum = 0, i;
    uint16_t crc;
    Write32(sector + 12, lba);
    crc = TagCrc(sector + 16, Read16(sector + 10));
    Write16(sector + 8, crc);
    sector[4] = 0;
    for (i = 0; i < 16; ++i)
        if (i != 4) checksum += sector[i];
    sector[4] = (uint8_t)checksum;
}

static UDFS_RECOVERY_PROBE_STATUS
ReadSector(const UDFS_RECOVERY_READER *reader, UDFS_RECOVERY_PROBE *result,
           uint32_t lba, uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE])
{
    if (lba >= reader->SectorCount) {
        result->FailureLba = lba;
        return UdfsRecoveryProbeCorrupt;
    }
    if (reader->ReadSector(reader->Context, lba, sector) != 0) {
        result->FailureLba = lba;
        return UdfsRecoveryProbeIoError;
    }
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
ValidateTag(const uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE],
            uint16_t kind, uint32_t location)
{
    uint32_t i, checksum = 0;
    uint16_t crcLength = Read16(sector + 10);

    if (Read16(sector) != kind || Read32(sector + 12) != location ||
        (Read16(sector + 2) != 2 && Read16(sector + 2) != 3))
        return UdfsRecoveryProbeCorrupt;
    if (crcLength > UDFS_RECOVERY_SECTOR_SIZE - 16)
        return UdfsRecoveryProbeUnsupported;
    for (i = 0; i < 16; ++i)
        if (i != 4) checksum += sector[i];
    if ((uint8_t)checksum != sector[4] ||
        TagCrc(sector + 16, crcLength) != Read16(sector + 8))
        return UdfsRecoveryProbeCorrupt;
    return UdfsRecoveryProbeValidLayout;
}

/* Only the LVID tag may be normalized, and only for the final OPEN descriptor
 * linked from a valid CLOSED predecessor. A torn 16-byte write can replace
 * the complete tag while leaving the old OPEN body intact. In that case all
 * immutable body bytes must still equal the predecessor, and the namespace
 * and allocation graph must validate before any repair is planned. */
static int
StableLvidBodyMatches(const uint8_t *previous, const uint8_t *current)
{
    return RecoveryDifference(previous + 48, current + 48, 32) == 0 &&
           RecoveryDifference(previous + 88, current + 88, 40) == 0 &&
           RecoveryDifference(previous + 136, current + 136,
                              UDFS_RECOVERY_SECTOR_SIZE - 136) == 0;
}

static UDFS_RECOVERY_PROBE_STATUS
NormalizeOpenLvidTag(uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE],
                     uint32_t lba, uint8_t allowMode,
                     const uint8_t *previousClosed, uint8_t *repairedMode)
{
    UDFS_RECOVERY_PROBE_STATUS status = ValidateTag(sector, 9, lba);
    uint32_t i, checksum = 0;
    uint8_t oldChecksum, oldCrcLow, oldCrcHigh;
    uint16_t crc;

    if (repairedMode) *repairedMode = 0;
    if (status != UdfsRecoveryProbeCorrupt || !allowMode || allowMode > 2)
        return status;
    if (Read16(sector + 10) < 136 - 16 ||
        Read32(sector + 28) != 0 ||
        Read32(sector + 32) != 0 || Read32(sector + 36) != 0)
        return status;
    for (i = 0; i < 16; ++i)
        if (i != 4) checksum += sector[i];
    oldChecksum = sector[4];
    if (oldChecksum != (uint8_t)checksum) {
        sector[4] = (uint8_t)checksum;
        status = ValidateTag(sector, 9, lba);
        if (status == UdfsRecoveryProbeValidLayout) {
            if (repairedMode) *repairedMode = 1;
            return status;
        }
        sector[4] = oldChecksum;
        return status;
    }

    if (allowMode != 2 || !previousClosed ||
        ValidateTag(previousClosed, 9, Read32(previousClosed + 12)) !=
            UdfsRecoveryProbeValidLayout ||
        Read32(previousClosed + 28) != 1 ||
        Read32(previousClosed + 32) != UDFS_RECOVERY_SECTOR_SIZE ||
        Read32(previousClosed + 36) != lba ||
        Read16(previousClosed + 2) != Read16(sector + 2) ||
        Read16(previousClosed + 6) != Read16(sector + 6) ||
        Read16(previousClosed + 10) != Read16(sector + 10) ||
        Read64(sector + 40) < Read64(previousClosed + 40) ||
        !StableLvidBodyMatches(previousClosed, sector))
        return status;
    oldCrcLow = sector[8];
    oldCrcHigh = sector[9];
    crc = TagCrc(sector + 16, Read16(sector + 10));
    sector[8] = (uint8_t)crc;
    sector[9] = (uint8_t)(crc >> 8);
    checksum = 0;
    for (i = 0; i < 16; ++i)
        if (i != 4) checksum += sector[i];
    sector[4] = (uint8_t)checksum;
    status = ValidateTag(sector, 9, lba);
    if (status != UdfsRecoveryProbeValidLayout) {
        sector[4] = oldChecksum;
        sector[8] = oldCrcLow;
        sector[9] = oldCrcHigh;
        return status;
    }
    if (repairedMode) *repairedMode = 2;
    return status;
}

/* The two unlinked journal slots are inside the already allocated integrity
 * extent. A slot is useful only for the linked final LVID it names. The
 * generation is CRC protected and never inferred from a damaged final tag. */
static int
JournalOpenValid(const uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE],
                 uint32_t slotLba, uint32_t finalLba,
                 const uint8_t previousClosed[UDFS_RECOVERY_SECTOR_SIZE])
{
    return ValidateTag(sector, 9, slotLba) ==
               UdfsRecoveryProbeValidLayout &&
           Read16(sector + 10) == UDFS_JOURNAL_CRC_LENGTH &&
           Read32(sector + 28) == 0 &&
           Read32(sector + 32) == 0 && Read32(sector + 36) == 0 &&
           Read32(sector + 72) == 2 &&
           Read32(sector + 76) == UDFS_JOURNAL_IMP_LENGTH &&
           RecoveryDifference(sector + 142, UdfsJournalMagic, 8) == 0 &&
           Read64(sector + 150) != 0 &&
           Read32(sector + 158) == finalLba &&
           Read32(sector + 162) <= 1 &&
           Read16(sector + 2) == Read16(previousClosed + 2) &&
           Read16(sector + 6) == Read16(previousClosed + 6) &&
           Read32(sector + 88) == Read32(previousClosed + 88) &&
           Read64(sector + 40) >= Read64(previousClosed + 40);
}

static int
JournalStableBodyMatches(const uint8_t *raw, const uint8_t *journal)
{
    return RecoveryDifference(raw + 48, journal + 48, 24) == 0 &&
           RecoveryDifference(raw + 88, journal + 88, 4) == 0 &&
           RecoveryDifference(raw + 96, journal + 96, 32) == 0 &&
           RecoveryDifference(raw + 136, journal + 136, 6) == 0 &&
           RecoveryDifference(raw + 166, journal + 166,
                              UDFS_RECOVERY_SECTOR_SIZE - 166) == 0;
}

static int
ValidLvidRecordingTime(const uint8_t *time)
{
    uint16_t year = Read16(time + 2);
    return year >= 1 && year <= 9999 &&
           time[4] >= 1 && time[4] <= 12 &&
           time[5] >= 1 && time[5] <= 31 &&
           time[6] <= 23 && time[7] <= 59 &&
           time[8] <= 59 && time[9] <= 99 &&
           time[10] <= 99 && time[11] <= 99;
}

/* The active OPEN target and its journal were already compared byte for
 * byte. Only the reserved alternate slot can be replaced. A short write
 * may have left a new tag header over its old generation's intact body.
 * Rebuild that old tag for validation; then require every observed header
 * byte to come from a single prefix of the prepared new header followed
 * by the reconstructed old header. The first seven time bytes must still
 * match the current checkpoint; a conflicting inactive body is not
 * silently discarded. The caller provides scratch to avoid more stack. */
static int
TornInactiveJournalReplaceable(
    const uint8_t before[UDFS_RECOVERY_SECTOR_SIZE],
    const uint8_t after[UDFS_RECOVERY_SECTOR_SIZE],
    const uint8_t active[UDFS_RECOVERY_SECTOR_SIZE],
    uint32_t slotLba, uint32_t finalLba,
    const uint8_t precedingClosed[UDFS_RECOVERY_SECTOR_SIZE],
    uint8_t scratch[UDFS_RECOVERY_SECTOR_SIZE])
{
    uint32_t split;
    uint64_t oldGeneration;

    if (RecoveryDifference(before, after, 24) == 0) return 1;
    if (RecoveryDifference(before + 16, after + 16, 7)) return 0;
    memcpy(scratch, before, UDFS_RECOVERY_SECTOR_SIZE);
    RetagLvid(scratch, slotLba);
    oldGeneration = Read64(scratch + 150);
    if (!JournalOpenValid(scratch, slotLba, finalLba,
                          precedingClosed) ||
        !ValidLvidRecordingTime(scratch + 16) ||
        oldGeneration == UINT64_MAX ||
        oldGeneration + 1 != Read64(active + 150) ||
        !JournalStableBodyMatches(scratch, active))
        return 0;
    for (split = 0; split <= 16; ++split)
        if (!RecoveryDifference(before, after, split) &&
            !RecoveryDifference(before + split, scratch + split,
                                16 - split))
            return 1;
    return 0;
}

static int
JournalHeaderMatches(const uint8_t *raw, const uint8_t *journal,
                     uint32_t finalLba)
{
    uint8_t expected[UDFS_RECOVERY_SECTOR_SIZE];
    memcpy(expected, journal, UDFS_RECOVERY_SECTOR_SIZE);
    RetagLvid(expected, finalLba);
    if (RecoveryDifference(raw, expected, 16) == 0) return 1;
    Write32(expected + 28, 1);
    RetagLvid(expected, finalLba);
    return RecoveryDifference(raw, expected, 16) == 0;
}

/* The final close may tear after its valid tag and CLOSED type reached disk.
 * The preceding durable OPEN journal owns the exact unique-ID high-water
 * mark. Never guess bytes from the 16-bit CRC or use an unlinked sector as a
 * checkpoint until its target and immutable body have been checked. */
static UDFS_RECOVERY_PROBE_STATUS
NormalizeLvidFromJournal(
    uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE], uint32_t finalLba,
    const uint8_t journal[UDFS_RECOVERY_SECTOR_SIZE],
    uint32_t journalLba,
    const uint8_t *previousJournal, uint8_t *repairedMode)
{
    uint8_t normalized[UDFS_RECOVERY_SECTOR_SIZE];
    UDFS_RECOVERY_PROBE_STATUS rawStatus =
        ValidateTag(sector, 9, finalLba);
    int staleValidOpen = rawStatus == UdfsRecoveryProbeValidLayout &&
                         Read32(sector + 28) == 0;

    if (repairedMode) *repairedMode = 0;
    if ((!staleValidOpen &&
         rawStatus != UdfsRecoveryProbeCorrupt) ||
        ValidateTag(journal, 9, journalLba) !=
            UdfsRecoveryProbeValidLayout ||
        Read16(sector) != 9 || Read32(sector + 12) != finalLba ||
        Read16(sector + 2) != Read16(journal + 2) ||
        Read16(sector + 6) != Read16(journal + 6) ||
        (Read16(sector + 10) != UDFS_JOURNAL_CRC_LENGTH &&
         (!staleValidOpen || Read16(sector + 10) !=
              UDFS_LEGACY_LVID_CRC_LENGTH)) ||
        Read32(sector + 28) > 1 ||
        Read32(sector + 32) != 0 || Read32(sector + 36) != 0 ||
        Read32(sector + 72) != 2 ||
        (Read32(sector + 76) != 46 &&
         Read32(sector + 76) != UDFS_JOURNAL_IMP_LENGTH) ||
        Read64(sector + 40) > Read64(journal + 40) ||
        !ValidLvidRecordingTime(sector + 16) ||
        !JournalStableBodyMatches(sector, journal))
        return UdfsRecoveryProbeCorrupt;

    memcpy(normalized, journal, UDFS_RECOVERY_SECTOR_SIZE);
    RetagLvid(normalized, finalLba);
    if (staleValidOpen) {
        uint16_t oldLength = Read32(sector + 76) ==
            UDFS_JOURNAL_IMP_LENGTH ? UDFS_JOURNAL_CRC_LENGTH :
            UDFS_LEGACY_LVID_CRC_LENGTH;
        uint64_t oldGeneration = oldLength ==
            UDFS_JOURNAL_CRC_LENGTH ? Read64(sector + 150) : 0;
        uint64_t newGeneration = Read64(journal + 150);
        uint64_t gap = oldGeneration <= newGeneration ?
                       newGeneration - oldGeneration : 0;
        if (Read16(sector + 10) != oldLength ||
            (oldLength == UDFS_JOURNAL_CRC_LENGTH &&
             (!oldGeneration ||
              RecoveryDifference(sector + 142, UdfsJournalMagic, 8) ||
              Read32(sector + 158) != finalLba ||
              Read32(sector + 162) > 1)) ||
            oldGeneration == UINT64_MAX ||
            (gap != 1 &&
             !(gap == 2 && Read32(journal + 162) == 1)))
            return UdfsRecoveryProbeCorrupt;
        if (previousJournal && gap == 1) {
            uint8_t expected[UDFS_RECOVERY_SECTOR_SIZE];
            memcpy(expected, previousJournal, sizeof(expected));
            RetagLvid(expected, finalLba);
            if (RecoveryDifference(sector, expected,
                                   sizeof(expected)))
                return UdfsRecoveryProbeCorrupt;
        }
    } else if (!JournalHeaderMatches(sector, journal, finalLba) &&
               (!previousJournal ||
                !JournalHeaderMatches(sector, previousJournal, finalLba)))
        return UdfsRecoveryProbeCorrupt;
    if (ValidateTag(normalized, 9, finalLba) !=
        UdfsRecoveryProbeValidLayout)
        return UdfsRecoveryProbeCorrupt;
    memcpy(sector, normalized, UDFS_RECOVERY_SECTOR_SIZE);
    if (repairedMode) *repairedMode = 3;
    return UdfsRecoveryProbeValidLayout;
}

/* A split mirror is recoverable only when both complete, CRC-valid directory
 * FEs differ solely in the link count and its tag CRC/checksum.  The
 * namespace walk later decides which count is correct; no copy wins merely
 * because it is primary or newer-looking. */
static int
DirectoryFePairDiffersOnlyInLinkCount(
    const uint8_t mainSector[UDFS_RECOVERY_SECTOR_SIZE],
    const uint8_t mirror[UDFS_RECOVERY_SECTOR_SIZE], uint32_t lbn)
{
    uint16_t kind = Read16(mainSector), a = Read16(mainSector + 48);
    uint16_t b = Read16(mirror + 48);
    uint32_t i;
    if ((kind != 261 && kind != 266) || Read16(mirror) != kind ||
        mainSector[27] != 4 || mirror[27] != 4 ||
        Read16(mainSector + 10) < 50 - 16 ||
        Read16(mirror + 10) < 50 - 16 ||
        ValidateTag(mainSector, kind, lbn) != UdfsRecoveryProbeValidLayout ||
        ValidateTag(mirror, kind, lbn) != UdfsRecoveryProbeValidLayout ||
        (a + 1 != b && b + 1 != a))
        return 0;
    for (i = 0; i < UDFS_RECOVERY_SECTOR_SIZE; ++i)
        if (i != 4 && i != 8 && i != 9 && i != 48 && i != 49 &&
            mainSector[i] != mirror[i])
            return 0;
    return 1;
}

static int
ValidFidRecordInSector(const uint8_t *fid, uint32_t available,
                       uint32_t lbn, uint32_t *recordLength)
{
    uint32_t raw, length, crcLength, checksum = 0, i;
    if (available < 40 || Read16(fid) != 257 ||
        (Read16(fid + 2) != 2 && Read16(fid + 2) != 3) ||
        Read16(fid + 16) != 1 || Read32(fid + 12) != lbn ||
        (fid[18] & ~0x1fU))
        return 0;
    raw = 38 + Read16(fid + 36) + fid[19];
    length = (raw + 3) & ~3U;
    crcLength = Read16(fid + 10);
    if (length < 40 || length > available ||
        crcLength < raw - 16 || crcLength > length - 16)
        return 0;
    for (i = 0; i < 16; ++i)
        if (i != 4) checksum += fid[i];
    if ((uint8_t)checksum != fid[4] ||
        TagCrc(fid + 16, crcLength) != Read16(fid + 8))
        return 0;
    *recordLength = length;
    return 1;
}

/* One complete directory FID changes from live to deleted.  A torn sector,
 * changed name or neighboring record is not an eligible recovery hint. */
static int
DirectoryFidPairLiveDeleted(
    const uint8_t mainSector[UDFS_RECOVERY_SECTOR_SIZE],
    const uint8_t mirror[UDFS_RECOVERY_SECTOR_SIZE],
    uint32_t lbn, int *liveOnMirror, uint32_t *childLbn,
    uint32_t *recordOffset)
{
    uint32_t offset, matches = 0;
    for (offset = 0; offset <= UDFS_RECOVERY_SECTOR_SIZE - 40;
         offset += 4) {
        const uint8_t *a = mainSector + offset, *b = mirror + offset;
        const uint8_t *live, *deleted;
        uint32_t aLength, bLength, j;
        int liveMirror;
        if (!ValidFidRecordInSector(a,
                 UDFS_RECOVERY_SECTOR_SIZE - offset, lbn, &aLength) ||
            !ValidFidRecordInSector(b,
                 UDFS_RECOVERY_SECTOR_SIZE - offset, lbn, &bLength) ||
            aLength != bLength)
            continue;
        if (!(a[18] & 4) && (b[18] & 4)) {
            live = a; deleted = b; liveMirror = 0;
        } else if ((a[18] & 4) && !(b[18] & 4)) {
            live = b; deleted = a; liveMirror = 1;
        } else continue;
        if (!(live[18] & 2) || (live[18] & 8) ||
            deleted[18] != (uint8_t)(live[18] | 4) ||
            (Read32(live + 20) >> 30) ||
            (Read32(live + 20) & 0x3fffffff) !=
                UDFS_RECOVERY_SECTOR_SIZE ||
            Read16(live + 28) != 1 ||
            !Read32(live + 24))
            continue;
        for (j = 20; j < 36; ++j)
            if (deleted[j]) break;
        if (j != 36) continue;
        for (j = 0; j < UDFS_RECOVERY_SECTOR_SIZE; ++j) {
            if (j >= offset && j < offset + aLength) {
                uint32_t local = j - offset;
                if (local == 4 || local == 8 || local == 9 ||
                    local == 18 || (local >= 20 && local < 36))
                    continue;
            }
            if (mainSector[j] != mirror[j]) break;
        }
        if (j != UDFS_RECOVERY_SECTOR_SIZE || ++matches != 1)
            return 0;
        *liveOnMirror = liveMirror;
        *childLbn = Read32(live + 24);
        *recordOffset = offset;
    }
    return matches == 1;
}

static UDFS_RECOVERY_PROBE_STATUS
ReadFileExtents(const UDFS_RECOVERY_READER *reader,
                UDFS_RECOVERY_PROBE *result, uint32_t feLbn,
                uint8_t expectedType, RECOVERY_EXTENT_LIST *extents)
{
    uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE];
    uint32_t lba, attrLength, adLength, start, offset, mode;
    UDFS_RECOVERY_PROBE_STATUS status;

    if (feLbn >= result->PartitionBlocks)
        return UdfsRecoveryProbeCorrupt;
    lba = result->PartitionStart + feLbn;
    status = ReadSector(reader, result, lba, sector);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    status = ValidateTag(sector, 266, feLbn);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    mode = Read16(sector + 34) & 7;
    if (sector[27] != expectedType ||
        (mode != 0 && !(expectedType == 252 && mode == 3)))
        return UdfsRecoveryProbeUnsupported;

    ResetExtents(extents);
    extents->FileBytes = Read64(sector + 56);
    attrLength = Read32(sector + 208);
    adLength = Read32(sector + 212);
    if (attrLength > UDFS_RECOVERY_SECTOR_SIZE - 216 ||
        adLength > UDFS_RECOVERY_SECTOR_SIZE - 216 - attrLength ||
        (mode == 0 && (adLength & 7)))
        return UdfsRecoveryProbeCorrupt;
    start = 216 + attrLength;
    if (Read16(sector + 10) < start + adLength - 16)
        return UdfsRecoveryProbeCorrupt;

    /* An initially small metadata bitmap is an SBD embedded in its own EFE.
     * Keep the FE sector as the sole physical owner and map bitmap bytes to
     * its in-ICB payload. The EFE CRC must cover every payload byte. */
    if (mode == 3) {
        if (extents->FileBytes < 24 || extents->FileBytes != adLength ||
            start + extents->FileBytes > UDFS_RECOVERY_SECTOR_SIZE)
            return UdfsRecoveryProbeCorrupt;
        extents->Inline = 1;
        extents->InlineFeLbn = feLbn;
        extents->InlineOffset = start;
        return UdfsRecoveryProbeValidLayout;
    }

    for (offset = 0; offset < adLength; offset += 8) {
        uint32_t encoded = Read32(sector + start + offset);
        uint32_t bytes = encoded & 0x3fffffff;
        uint32_t blockLbn = Read32(sector + start + offset + 4);
        uint32_t blocks;
        if (!bytes) {
            if (encoded || blockLbn) return UdfsRecoveryProbeCorrupt;
            continue;
        }
        if (encoded >> 30 ||
            (expectedType != 252 &&
             (bytes & (UDFS_RECOVERY_SECTOR_SIZE - 1))))
            return UdfsRecoveryProbeUnsupported;
        blocks = (bytes + UDFS_RECOVERY_SECTOR_SIZE - 1) /
                 UDFS_RECOVERY_SECTOR_SIZE;
        if (!blocks || blockLbn >= result->PartitionBlocks ||
            blocks > result->PartitionBlocks - blockLbn ||
            extents->Count == extents->Capacity ||
            blocks > UINT32_MAX - extents->TotalBlocks)
            return UdfsRecoveryProbeCorrupt;
        extents->Items[extents->Count].Lbn = blockLbn;
        extents->Items[extents->Count].Blocks = blocks;
        extents->Items[extents->Count].Bytes = bytes;
        extents->Items[extents->Count].Ref = 0;
        extents->Count++;
        extents->TotalBlocks += blocks;
    }
    if (!extents->Count ||
        (uint64_t)extents->TotalBlocks * UDFS_RECOVERY_SECTOR_SIZE <
            extents->FileBytes)
        return UdfsRecoveryProbeCorrupt;
    return UdfsRecoveryProbeValidLayout;
}

static int
RangesOverlap(const RECOVERY_EXTENT *left, const RECOVERY_EXTENT *right)
{
    return left->Lbn < right->Lbn + right->Blocks &&
           right->Lbn < left->Lbn + left->Blocks;
}

static UDFS_RECOVERY_PROBE_STATUS
MapExtentByte(const UDFS_RECOVERY_PROBE *result,
              const RECOVERY_EXTENT_LIST *extents, uint64_t offset,
              uint32_t *lba, uint32_t *sectorOffset)
{
    uint32_t i;
    if (offset >= extents->FileBytes) return UdfsRecoveryProbeCorrupt;
    if (extents->Inline) {
        if (extents->InlineFeLbn >= result->PartitionBlocks ||
            extents->InlineOffset >= UDFS_RECOVERY_SECTOR_SIZE ||
            offset >= UDFS_RECOVERY_SECTOR_SIZE - extents->InlineOffset)
            return UdfsRecoveryProbeCorrupt;
        *lba = result->PartitionStart + extents->InlineFeLbn;
        *sectorOffset = extents->InlineOffset + (uint32_t)offset;
        return UdfsRecoveryProbeValidLayout;
    }
    for (i = 0; i < extents->Count; ++i) {
        if (offset < extents->Items[i].Bytes) {
            *lba = result->PartitionStart + extents->Items[i].Lbn +
                   (uint32_t)(offset / UDFS_RECOVERY_SECTOR_SIZE);
            *sectorOffset = (uint32_t)(offset % UDFS_RECOVERY_SECTOR_SIZE);
            return UdfsRecoveryProbeValidLayout;
        }
        offset -= extents->Items[i].Bytes;
    }
    return UdfsRecoveryProbeCorrupt;
}

static UDFS_RECOVERY_PROBE_STATUS
ReadCachedByte(const UDFS_RECOVERY_READER *reader,
               UDFS_RECOVERY_PROBE *result, RECOVERY_SECTOR_CACHE *cache,
               uint32_t lba, uint32_t offset, uint8_t *value)
{
    UDFS_RECOVERY_PROBE_STATUS status;
    if (offset >= UDFS_RECOVERY_SECTOR_SIZE)
        return UdfsRecoveryProbeCorrupt;
    if (!cache->Valid || cache->Lba != lba) {
        status = ReadSector(reader, result, lba, cache->Data);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        cache->Lba = lba;
        cache->Valid = 1;
    }
    *value = cache->Data[offset];
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
MapMetadataBlock(const UDFS_RECOVERY_PROBE *result,
                 const RECOVERY_EXTENT_LIST *extents, uint32_t block,
                 uint32_t *lba)
{
    uint32_t i;
    if (block >= result->MetadataBlocks) return UdfsRecoveryProbeCorrupt;
    for (i = 0; i < extents->Count; ++i) {
        if (block < extents->Items[i].Blocks) {
            *lba = result->PartitionStart + extents->Items[i].Lbn + block;
            return UdfsRecoveryProbeValidLayout;
        }
        block -= extents->Items[i].Blocks;
    }
    return UdfsRecoveryProbeCorrupt;
}

typedef struct _RECOVERY_GRAPH_ENTRY {
    uint32_t Lbn;
    uint32_t Bytes;
    uint32_t ParentLbn;
    uint64_t StreamOwnerUniqueId;
    uint16_t Ref;
    uint16_t ParentRef;
    uint8_t Directory;
    /* 0: ordinary namespace, 1: EFE stream directory, 2: named stream. */
    uint8_t StreamRole;
} RECOVERY_GRAPH_ENTRY;

typedef struct _RECOVERY_NAME_SLOT {
    uint32_t Hash;
    uint32_t OffsetPlusOne;
} RECOVERY_NAME_SLOT;

/* Physical capacity must not require three dense nonpaged bitmaps. A page
 * with no Data is uniformly clear (SetBits == 0) or set (SetBits == its
 * valid bit count). Only mixed pages require a 4 KiB allocation. */
#define RECOVERY_BITMAP_PAGE_SHIFT 15U
#define RECOVERY_BITMAP_PAGE_BITS (1U << RECOVERY_BITMAP_PAGE_SHIFT)
#define RECOVERY_BITMAP_PAGE_BYTES (RECOVERY_BITMAP_PAGE_BITS / 8U)

typedef struct _RECOVERY_BITMAP_PAGE {
    uint8_t *Data;
    uint32_t SetBits;
} RECOVERY_BITMAP_PAGE;

typedef struct _RECOVERY_BITMAP {
    RECOVERY_BITMAP_PAGE *Pages;
    uint32_t BitCount;
    uint32_t PageCount;
} RECOVERY_BITMAP;

typedef struct _RECOVERY_GRAPH {
    const UDFS_RECOVERY_READER *Reader;
    UDFS_RECOVERY_PROBE *Result;
    const RECOVERY_EXTENT_LIST *Metadata;
    const RECOVERY_EXTENT_LIST *BitmapFile;
    RECOVERY_BITMAP PhysicalOwned;
    uint8_t *MetadataOwned;
    RECOVERY_BITMAP PhysicalVisited;
    uint8_t *MetadataVisited;
    RECOVERY_BITMAP PhysicalDirs;
    uint8_t *MetadataDirs;
    uint8_t *MetadataInlineFileFes;
    RECOVERY_GRAPH_ENTRY *Queue;
    RECOVERY_EXTENT_LIST *ScratchExtents;
    uint32_t QueueCount;
    uint32_t QueueCapacity;
    uint32_t FileReferences;
    uint64_t *UniqueIds;
    uint32_t UniqueIdCount;
    uint32_t UniqueIdCapacity;
    RECOVERY_SECTOR_CACHE PhysicalBitmapCache;
    RECOVERY_SECTOR_CACHE MetadataBitmapCache;
} RECOVERY_GRAPH;

/* Remember where the namespace walk first failed. The line number names the
 * exact check in this file, so a refused recovery can be diagnosed from the
 * mount log without a copy of the disk. Later failures keep the first one. */
static UDFS_RECOVERY_PROBE_STATUS
GraphFail(RECOVERY_GRAPH *graph, UDFS_RECOVERY_PROBE_STATUS status,
          uint32_t line)
{
    if (status != UdfsRecoveryProbeValidLayout &&
        !graph->Result->FailureLine)
        graph->Result->FailureLine = line;
    return status;
}
#define GRAPH_FAIL(status) GraphFail(graph, (status), __LINE__)

static UDFS_RECOVERY_PROBE_STATUS
GraphMetadataBitIsFree(RECOVERY_GRAPH *graph, uint32_t lbn, int *freeBit)
{
    uint32_t lba, offset;
    uint8_t value;
    UDFS_RECOVERY_PROBE_STATUS status;
    if (lbn >= graph->Result->MetadataBlocks)
        return UdfsRecoveryProbeCorrupt;
    status = MapExtentByte(graph->Result, graph->BitmapFile,
                          24ULL + (lbn >> 3), &lba, &offset);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    status = ReadCachedByte(graph->Reader, graph->Result,
                            &graph->MetadataBitmapCache,
                            lba, offset, &value);
    if (status == UdfsRecoveryProbeValidLayout)
        *freeBit = (value >> (lbn & 7)) & 1;
    return status;
}

static UDFS_RECOVERY_PROBE_STATUS
GraphRecordUniqueId(RECOVERY_GRAPH *graph, uint64_t id, int root)
{
    uint32_t slot, i, capacity;
    uint64_t *replacement;
    if (!id)
        return root ? UdfsRecoveryProbeValidLayout : UdfsRecoveryProbeCorrupt;
    if (root || (uint32_t)id < 16)
        return UdfsRecoveryProbeCorrupt;
    if (graph->UniqueIdCount * 2 >= graph->UniqueIdCapacity) {
        if (graph->UniqueIdCapacity >= 2 * 1024 * 1024)
            return UdfsRecoveryProbeUnsupported;
        capacity = graph->UniqueIdCapacity ? graph->UniqueIdCapacity * 2 : 1024;
        replacement = (uint64_t *)graph->Reader->Allocate(
            graph->Reader->Context, (size_t)capacity * sizeof(*replacement));
        if (!replacement) return UdfsRecoveryProbeNoMemory;
        memset(replacement, 0, (size_t)capacity * sizeof(*replacement));
        for (i = 0; i < graph->UniqueIdCapacity; ++i) {
            uint64_t old = graph->UniqueIds[i];
            if (!old) continue;
            slot = ((uint32_t)old ^ (uint32_t)(old >> 32)) * 2654435761U;
            slot &= capacity - 1;
            while (replacement[slot]) slot = (slot + 1) & (capacity - 1);
            replacement[slot] = old;
        }
        if (graph->UniqueIds)
            graph->Reader->Free(graph->Reader->Context, graph->UniqueIds);
        graph->UniqueIds = replacement;
        graph->UniqueIdCapacity = capacity;
    }
    slot = ((uint32_t)id ^ (uint32_t)(id >> 32)) * 2654435761U;
    slot &= graph->UniqueIdCapacity - 1;
    while (graph->UniqueIds[slot]) {
        if (graph->UniqueIds[slot] == id) return UdfsRecoveryProbeCorrupt;
        slot = (slot + 1) & (graph->UniqueIdCapacity - 1);
    }
    graph->UniqueIds[slot] = id;
    graph->UniqueIdCount++;
    return UdfsRecoveryProbeValidLayout;
}

/* The recovery namespace checker deliberately supports ASCII names only.
 * Ordinary clean mounts retain the full driver's Unicode/hard-link support.
 * Do not approximate an unknown Unicode collation during crash recovery. */
static UDFS_RECOVERY_PROBE_STATUS
RecoveryName(const uint8_t *fid, uint8_t name[255], uint32_t *length,
             uint32_t *hash)
{
    const uint8_t *encoded = fid + 38 + Read16(fid + 36);
    uint32_t i, step, bytes = fid[19];
    if (bytes < 2 || (encoded[0] != 8 && encoded[0] != 16))
        return UdfsRecoveryProbeCorrupt;
    step = encoded[0] == 16 ? 2 : 1;
    if ((bytes - 1) % step) return UdfsRecoveryProbeCorrupt;
    *length = 0;
    *hash = 2166136261U;
    for (i = 1; i < bytes; i += step) {
        uint32_t value = step == 2 ?
            ((uint32_t)encoded[i] << 8) | encoded[i + 1] : encoded[i];
        if (value > 127) return UdfsRecoveryProbeUnsupported;
        if (!value || value == '/' || value == '\\')
            return UdfsRecoveryProbeCorrupt;
        if (value >= 'a' && value <= 'z') value -= 'a' - 'A';
        name[(*length)++] = (uint8_t)value;
        *hash = (*hash ^ value) * 16777619U;
    }
    if ((*length == 1 && name[0] == '.') ||
        (*length == 2 && name[0] == '.' && name[1] == '.'))
        return UdfsRecoveryProbeCorrupt;
    return UdfsRecoveryProbeValidLayout;
}

static int
BitIsSet(const uint8_t *bits, uint32_t index)
{
    return (bits[index >> 3] >> (index & 7)) & 1;
}

static void
SetBit(uint8_t *bits, uint32_t index)
{
    bits[index >> 3] |= (uint8_t)(1U << (index & 7));
}

static uint32_t
CountByteBits(uint8_t value)
{
    uint32_t bits = value;
    bits -= (bits >> 1) & 0x55U;
    bits = (bits & 0x33U) + ((bits >> 2) & 0x33U);
    return (bits + (bits >> 4)) & 0x0fU;
}

static UDFS_RECOVERY_PROBE_STATUS
BitmapInitialize(const UDFS_RECOVERY_READER *reader,
                 RECOVERY_BITMAP *bitmap, uint32_t bitCount)
{
    size_t bytes;
    bitmap->BitCount = bitCount;
    bitmap->PageCount = (uint32_t)(((uint64_t)bitCount +
        RECOVERY_BITMAP_PAGE_BITS - 1) >> RECOVERY_BITMAP_PAGE_SHIFT);
    bytes = (size_t)bitmap->PageCount * sizeof(*bitmap->Pages);
    bitmap->Pages = (RECOVERY_BITMAP_PAGE *)reader->Allocate(
        reader->Context, bytes);
    if (!bitmap->Pages) return UdfsRecoveryProbeNoMemory;
    memset(bitmap->Pages, 0, bytes);
    return UdfsRecoveryProbeValidLayout;
}

static void
BitmapRelease(const UDFS_RECOVERY_READER *reader, RECOVERY_BITMAP *bitmap)
{
    uint32_t i;
    if (!bitmap->Pages) return;
    for (i = 0; i < bitmap->PageCount; ++i)
        if (bitmap->Pages[i].Data)
            reader->Free(reader->Context, bitmap->Pages[i].Data);
    reader->Free(reader->Context, bitmap->Pages);
    memset(bitmap, 0, sizeof(*bitmap));
}

static int
BitmapBitIsSet(const RECOVERY_BITMAP *bitmap, uint32_t index)
{
    const RECOVERY_BITMAP_PAGE *page =
        &bitmap->Pages[index >> RECOVERY_BITMAP_PAGE_SHIFT];
    return page->Data ? BitIsSet(page->Data,
        index & (RECOVERY_BITMAP_PAGE_BITS - 1)) : page->SetBits != 0;
}

/* Byte indices never straddle a sparse page. The caller masks the unused
 * high bits of the final byte, including a uniformly set final page. */
static uint8_t
BitmapReadByte(const RECOVERY_BITMAP *bitmap, uint32_t byteIndex)
{
    const RECOVERY_BITMAP_PAGE *page =
        &bitmap->Pages[byteIndex / RECOVERY_BITMAP_PAGE_BYTES];
    return page->Data ? page->Data[byteIndex % RECOVERY_BITMAP_PAGE_BYTES] :
           (page->SetBits ? 0xff : 0);
}

static uint32_t
FirstByteBit(uint8_t value)
{
    uint32_t bit = 0;
    /* Used only for a nonzero mismatch mask. */
    while (!(value & 1)) {
        value >>= 1;
        bit++;
    }
    return bit;
}

static UDFS_RECOVERY_PROBE_STATUS
BitmapSetRange(const UDFS_RECOVERY_READER *reader, RECOVERY_BITMAP *bitmap,
               uint32_t first, uint32_t count, uint32_t *alreadySet)
{
    *alreadySet = 0;
    if (first > bitmap->BitCount || count > bitmap->BitCount - first)
        return UdfsRecoveryProbeCorrupt;
    while (count) {
        uint32_t pageIndex = first >> RECOVERY_BITMAP_PAGE_SHIFT;
        uint32_t offset = first & (RECOVERY_BITMAP_PAGE_BITS - 1);
        uint32_t valid = bitmap->BitCount -
            (pageIndex << RECOVERY_BITMAP_PAGE_SHIFT);
        uint32_t take, i, remaining;
        RECOVERY_BITMAP_PAGE *page = &bitmap->Pages[pageIndex];
        if (valid > RECOVERY_BITMAP_PAGE_BITS)
            valid = RECOVERY_BITMAP_PAGE_BITS;
        take = valid - offset;
        if (take > count) take = count;
        if (!offset && take == valid) {
            *alreadySet += page->SetBits;
            if (page->Data) reader->Free(reader->Context, page->Data);
            page->Data = NULL;
            page->SetBits = valid;
        } else if (!page->Data && page->SetBits) {
            *alreadySet += take;
        } else {
            if (!page->Data) {
                page->Data = (uint8_t *)reader->Allocate(reader->Context,
                    RECOVERY_BITMAP_PAGE_BYTES);
                if (!page->Data) return UdfsRecoveryProbeNoMemory;
                memset(page->Data, 0, RECOVERY_BITMAP_PAGE_BYTES);
            }
            i = offset;
            remaining = take;
            while (remaining) {
                uint32_t width = 8 - (i & 7);
                uint32_t old;
                uint8_t mask;
                if (width > remaining) width = remaining;
                mask = (uint8_t)(((1U << width) - 1) << (i & 7));
                old = CountByteBits(page->Data[i >> 3] & mask);
                *alreadySet += old;
                page->SetBits += width - old;
                page->Data[i >> 3] |= mask;
                i += width;
                remaining -= width;
            }
            if (page->SetBits == valid) {
                reader->Free(reader->Context, page->Data);
                page->Data = NULL;
            }
        }
        first += take;
        count -= take;
    }
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
GraphOwn(RECOVERY_GRAPH *graph, uint16_t ref, uint32_t lbn,
         uint32_t bytes)
{
    uint32_t blocks, limit, i;
    uint8_t *owned;
    uint32_t *count;
    if (!bytes) return UdfsRecoveryProbeValidLayout;
    blocks = (bytes + UDFS_RECOVERY_SECTOR_SIZE - 1) /
             UDFS_RECOVERY_SECTOR_SIZE;
    if (ref == 0) {
        uint32_t overlap;
        UDFS_RECOVERY_PROBE_STATUS status;
        limit = graph->Result->PartitionBlocks;
        if (lbn >= limit || blocks > limit - lbn)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        status = BitmapSetRange(graph->Reader, &graph->PhysicalOwned,
                                 lbn, blocks, &overlap);
        if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
        if (overlap > UINT32_MAX - graph->Result->OverlapBlocks)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        graph->Result->OverlapBlocks += overlap;
        graph->Result->OwnedPhysicalBlocks += blocks - overlap;
        return UdfsRecoveryProbeValidLayout;
    } else if (ref == 1) {
        limit = graph->Result->MetadataBlocks;
        owned = graph->MetadataOwned;
        count = &graph->Result->OwnedMetadataBlocks;
    } else {
        return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
    }
    if (lbn >= limit || blocks > limit - lbn)
        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    for (i = 0; i < blocks; ++i) {
        if (BitIsSet(owned, lbn + i))
            graph->Result->OverlapBlocks++;
        else {
            SetBit(owned, lbn + i);
            ++*count;
        }
    }
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
GraphReadLogical(RECOVERY_GRAPH *graph, uint16_t ref,
                 uint32_t lbn, uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE])
{
    UDFS_RECOVERY_PROBE_STATUS status;
    uint32_t physical;
    if (ref == 0) {
        if (lbn >= graph->Result->PartitionBlocks)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        physical = graph->Result->PartitionStart + lbn;
    } else if (ref == 1) {
        status = MapMetadataBlock(graph->Result, graph->Metadata,
                                  lbn, &physical);
        if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
        if (graph->Result->FidSplitCount &&
            lbn == graph->Result->FidSplitLbn) {
            if (physical != graph->Result->FidSplitMainLba)
                return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
            if (graph->Result->FidSplitLiveOnMirror)
                physical = graph->Result->FidSplitMirrorLba;
        }
    } else {
        return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
    }
    return GRAPH_FAIL(ReadSector(graph->Reader, graph->Result, physical, sector));
}

static UDFS_RECOVERY_PROBE_STATUS
GraphPush(RECOVERY_GRAPH *graph, uint16_t ref, uint32_t lbn,
          uint32_t bytes, int directory, uint16_t parentRef,
          uint32_t parentLbn, uint8_t streamRole,
          uint64_t streamOwnerUniqueId)
{
    RECOVERY_GRAPH_ENTRY *replacement;
    uint32_t capacity;
    if (graph->QueueCount == graph->QueueCapacity) {
        if (graph->QueueCapacity >= 1024 * 1024)
            return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
        capacity = graph->QueueCapacity ? graph->QueueCapacity * 2 : 1024;
        replacement = (RECOVERY_GRAPH_ENTRY *)graph->Reader->Allocate(
            graph->Reader->Context, (size_t)capacity * sizeof(*replacement));
        if (!replacement) return UdfsRecoveryProbeNoMemory;
        if (graph->Queue) {
            memcpy(replacement, graph->Queue,
                   (size_t)graph->QueueCount * sizeof(*replacement));
            graph->Reader->Free(graph->Reader->Context, graph->Queue);
        }
        graph->Queue = replacement;
        graph->QueueCapacity = capacity;
    }
    graph->Queue[graph->QueueCount].Ref = ref;
    graph->Queue[graph->QueueCount].Lbn = lbn;
    graph->Queue[graph->QueueCount].Bytes = bytes;
    graph->Queue[graph->QueueCount].ParentRef = parentRef;
    graph->Queue[graph->QueueCount].ParentLbn = parentLbn;
    graph->Queue[graph->QueueCount].Directory = directory ? 1 : 0;
    graph->Queue[graph->QueueCount].StreamRole = streamRole;
    graph->Queue[graph->QueueCount].StreamOwnerUniqueId =
        streamOwnerUniqueId;
    graph->QueueCount++;
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
GraphVisit(RECOVERY_GRAPH *graph, const RECOVERY_GRAPH_ENTRY *entry,
           int *alreadyVisited)
{
    uint8_t *visited = NULL, *directories = NULL;
    uint32_t limit;
    UDFS_RECOVERY_PROBE_STATUS status;
    if (entry->Ref == 0) {
        limit = graph->Result->PartitionBlocks;
    } else if (entry->Ref == 1) {
        visited = graph->MetadataVisited;
        directories = graph->MetadataDirs;
        limit = graph->Result->MetadataBlocks;
    } else return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
    if (entry->Lbn >= limit || entry->Bytes != UDFS_RECOVERY_SECTOR_SIZE)
        return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
    *alreadyVisited = entry->Ref == 0 ?
        BitmapBitIsSet(&graph->PhysicalVisited, entry->Lbn) :
        BitIsSet(visited, entry->Lbn);
    if (*alreadyVisited) {
        if (entry->StreamRole)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        int isDirectory = entry->Ref == 0 ?
            BitmapBitIsSet(&graph->PhysicalDirs, entry->Lbn) :
            BitIsSet(directories, entry->Lbn);
        if (entry->Directory || isDirectory != entry->Directory)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        return UdfsRecoveryProbeValidLayout;
    }
    if (entry->Ref == 0) {
        uint32_t overlap;
        status = BitmapSetRange(graph->Reader, &graph->PhysicalVisited,
                                 entry->Lbn, 1, &overlap);
        if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
        if (entry->Directory) {
            status = BitmapSetRange(graph->Reader, &graph->PhysicalDirs,
                                     entry->Lbn, 1, &overlap);
            if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
        }
    } else {
        SetBit(visited, entry->Lbn);
        if (entry->Directory) SetBit(directories, entry->Lbn);
    }
    if (entry->Directory) {
        graph->Result->DirectoriesVisited++;
    } else if (!entry->StreamRole) {
        /* UDFS counts namespace files in LVID, but neither its stream
         * directory nor named stream FEs.  Both are still visited and owned
         * by the graph above. */
        graph->Result->FilesVisited++;
    }
    return UdfsRecoveryProbeValidLayout;
}

/* Bound the work for one file's descriptor chain. Each continuation block
 * holds about 30 long ADs, so this allows far more extents than any file
 * the driver writes, while a damaged chain still ends quickly. */
#define RECOVERY_MAX_AD_CONTINUATIONS 65536U

static UDFS_RECOVERY_PROBE_STATUS
GraphParseFileAds(RECOVERY_GRAPH *graph, const RECOVERY_GRAPH_ENTRY *file,
                  const uint8_t fe[UDFS_RECOVERY_SECTOR_SIZE],
                  RECOVERY_EXTENT_LIST *dataExtents, uint32_t *inlineStart,
                  uint32_t *inlineLength, uint32_t *modeOut)
{
    uint8_t source[UDFS_RECOVERY_SECTOR_SIZE];
    uint32_t mode = Read16(fe + 34) & 7;
    uint32_t attrOffset, adOffset, dataOffset;
    uint32_t start, length, sourceStart, sourceLength, continuations = 0;
    /* Only directories and stream directories are read back through the
     * extent list. A regular file's extents are owned and counted only, so
     * a heavily fragmented file is not limited by the list capacity. */
    int keepExtents = file->Directory || file->StreamRole == 1;
    UDFS_RECOVERY_PROBE_STATUS status;
    uint64_t dataBytes = 0;

    if (Read16(fe) == 266) {
        attrOffset = 208; adOffset = 212; dataOffset = 216;
    } else if (Read16(fe) == 261) {
        attrOffset = 168; adOffset = 172; dataOffset = 176;
    } else return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    if (mode != 0 && mode != 1 && mode != 3)
        return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
    if (Read32(fe + attrOffset) > UDFS_RECOVERY_SECTOR_SIZE - dataOffset)
        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    start = dataOffset + Read32(fe + attrOffset);
    length = Read32(fe + adOffset);
    if (start > UDFS_RECOVERY_SECTOR_SIZE ||
        length > UDFS_RECOVERY_SECTOR_SIZE - start ||
        Read16(fe + 10) < start + length - 16)
        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    *modeOut = mode;
    *inlineStart = start;
    *inlineLength = length;
    ResetExtents(dataExtents);
    dataExtents->FileBytes = Read64(fe + 56);
    if (mode == 3)
        return dataExtents->FileBytes <= length ?
            UdfsRecoveryProbeValidLayout : GRAPH_FAIL(UdfsRecoveryProbeCorrupt);

    memcpy(source, fe, sizeof(source));
    sourceStart = start;
    sourceLength = length;
    for (;;) {
        uint32_t offset, step = mode == 0 ? 8 : 16;
        uint32_t nextLbn = 0, nextBytes = 0;
        uint16_t nextRef = 0;
        int haveNext = 0;
        uint32_t tail;
        if (sourceStart > UDFS_RECOVERY_SECTOR_SIZE ||
            sourceLength > UDFS_RECOVERY_SECTOR_SIZE - sourceStart ||
            Read16(source + 10) < sourceStart + sourceLength - 16)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        /* Older UDFS builds counted the zero padding after the last
         * descriptor of a continued FE block in L_AD. Accept only that
         * shape: a tail shorter than one descriptor, entirely zero. */
        tail = sourceLength % step;
        if (tail) {
            for (offset = sourceStart + sourceLength - tail;
                 offset < sourceStart + sourceLength; ++offset)
                if (source[offset])
                    return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
            sourceLength -= tail;
        }
        for (offset = sourceStart; offset < sourceStart + sourceLength;
             offset += step) {
            uint32_t encoded = Read32(source + offset);
            uint32_t bytes = encoded & 0x3fffffff;
            uint32_t kind = encoded >> 30;
            uint32_t lbn = Read32(source + offset + 4);
            uint16_t ref = mode == 0 ? file->Ref :
                           Read16(source + offset + 8);
            if (!bytes) continue;
            if (kind == 3) {
                if (haveNext || bytes < UDFS_RECOVERY_SECTOR_SIZE)
                    return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
                nextLbn = lbn;
                nextRef = ref;
                nextBytes = bytes;
                haveNext = 1;
            } else if (kind == 0 || kind == 1) {
                if (haveNext ||
                    (keepExtents &&
                     dataExtents->Count == dataExtents->Capacity))
                    return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
                /* An allocated but unrecorded AD owns its blocks.  It is
                 * valid in an ordinary file, but cannot supply FID bytes. */
                if (kind == 1 &&
                    (file->Directory || file->StreamRole == 1 ||
                     (bytes & (UDFS_RECOVERY_SECTOR_SIZE - 1))))
                    return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
                status = GraphOwn(graph, ref, lbn, bytes);
                if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
                if (dataExtents->Count < dataExtents->Capacity) {
                    RECOVERY_EXTENT *extent =
                        &dataExtents->Items[dataExtents->Count++];
                    extent->Ref = ref;
                    extent->Lbn = lbn;
                    extent->Bytes = bytes;
                    extent->Blocks = (bytes + UDFS_RECOVERY_SECTOR_SIZE - 1) /
                                     UDFS_RECOVERY_SECTOR_SIZE;
                }
                dataBytes += bytes;
            } else {
                return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
            }
        }
        /* That padding only exists in front of a continuation. */
        if (tail && !haveNext)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        if (!haveNext) break;
        if (++continuations > RECOVERY_MAX_AD_CONTINUATIONS ||
            nextBytes != UDFS_RECOVERY_SECTOR_SIZE)
            return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
        {
            /* A continuation block that is already owned is either a loop
             * in this chain or shared with another object. */
            uint32_t overlapBefore = graph->Result->OverlapBlocks;
            status = GraphOwn(graph, nextRef, nextLbn, nextBytes);
            if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
            if (graph->Result->OverlapBlocks != overlapBefore)
                return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        }
        status = GraphReadLogical(graph, nextRef, nextLbn, source);
        if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
        status = ValidateTag(source, 258, nextLbn);
        if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
        sourceStart = 24;
        sourceLength = Read32(source + 20);
    }
    return dataBytes >= dataExtents->FileBytes ?
        UdfsRecoveryProbeValidLayout : GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
}

static UDFS_RECOVERY_PROBE_STATUS
GraphReadDirectory(RECOVERY_GRAPH *graph,
                   const RECOVERY_GRAPH_ENTRY *file,
                   const uint8_t fe[UDFS_RECOVERY_SECTOR_SIZE],
                   const RECOVERY_EXTENT_LIST *extents,
                   uint32_t inlineStart, uint32_t mode,
                   uint8_t **directory, uint32_t *length)
{
    UDFS_RECOVERY_PROBE_STATUS status;
    uint32_t copied = 0, i;
    if (!extents->FileBytes || extents->FileBytes > 16 * 1024 * 1024)
        return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
    *length = (uint32_t)extents->FileBytes;
    *directory = (uint8_t *)graph->Reader->Allocate(graph->Reader->Context,
                                                     *length);
    if (!*directory) return UdfsRecoveryProbeNoMemory;
    if (mode == 3) {
        memcpy(*directory, fe + inlineStart, *length);
        return UdfsRecoveryProbeValidLayout;
    }
    for (i = 0; i < extents->Count && copied < *length; ++i) {
        const RECOVERY_EXTENT *extent = &extents->Items[i];
        uint32_t byteOffset;
        for (byteOffset = 0; byteOffset < extent->Bytes && copied < *length;
             byteOffset += UDFS_RECOVERY_SECTOR_SIZE) {
            uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE];
            uint32_t available = extent->Bytes - byteOffset;
            uint32_t take = *length - copied;
            if (take > available) take = available;
            if (take > UDFS_RECOVERY_SECTOR_SIZE)
                take = UDFS_RECOVERY_SECTOR_SIZE;
            status = GraphReadLogical(graph, extent->Ref,
                        extent->Lbn + byteOffset / UDFS_RECOVERY_SECTOR_SIZE,
                        sector);
            if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
            memcpy(*directory + copied, sector, take);
            copied += take;
        }
    }
    (void)file;
    return copied == *length ? UdfsRecoveryProbeValidLayout :
           GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
}

static UDFS_RECOVERY_PROBE_STATUS
GraphScanFids(RECOVERY_GRAPH *graph,
              const RECOVERY_GRAPH_ENTRY *directoryFile,
              const RECOVERY_EXTENT_LIST *extents, uint32_t mode,
              const uint8_t *directory, uint32_t length, uint16_t links,
              RECOVERY_NAME_SLOT *names, uint32_t nameCapacity)
{
    uint32_t offset = 0, parents = 0, subdirectories = 0;
    while (offset < length) {
        const uint8_t *fid = directory + offset;
        uint32_t rawLength, recordLength, crcLength, expectedLbn;
        uint32_t recordSectorOffset = UINT32_MAX;
        uint32_t checksum = 0, i;
        uint8_t attributes;
        UDFS_RECOVERY_PROBE_STATUS status;
        if (length - offset < 38) return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        rawLength = 38 + Read16(fid + 36) + fid[19];
        recordLength = (rawLength + 3) & ~3U;
        crcLength = Read16(fid + 10);
        attributes = fid[18];
        if (recordLength > length - offset ||
            Read16(fid) != 257 ||
            (Read16(fid + 2) != 2 && Read16(fid + 2) != 3) ||
            Read16(fid + 16) != 1 || (attributes & ~0x1fU) ||
            crcLength < rawLength - 16 ||
            crcLength > recordLength - 16)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        for (i = 0; i < 16; ++i)
            if (i != 4) checksum += fid[i];
        if ((uint8_t)checksum != fid[4] ||
            TagCrc(fid + 16, crcLength) != Read16(fid + 8))
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        if (mode == 3) {
            expectedLbn = directoryFile->Lbn;
        } else {
            uint32_t remaining = offset;
            for (i = 0; i < extents->Count; ++i) {
                if (remaining < extents->Items[i].Bytes) break;
                remaining -= extents->Items[i].Bytes;
            }
            if (i == extents->Count) return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
            expectedLbn = extents->Items[i].Lbn +
                          remaining / UDFS_RECOVERY_SECTOR_SIZE;
            recordSectorOffset =
                remaining % UDFS_RECOVERY_SECTOR_SIZE;
        }
        if (Read32(fid + 12) != expectedLbn)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        if (graph->Result->FidSplitCount &&
            expectedLbn == graph->Result->FidSplitLbn &&
            recordSectorOffset ==
                graph->Result->FidSplitRecordOffset) {
            if (graph->Result->FidSplitSeen ||
                !(attributes & 2) || (attributes & (4 | 8)) ||
                Read16(fid + 28) != 1 ||
                Read32(fid + 24) !=
                    graph->Result->FidSplitChildLbn)
                return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
            graph->Result->FidSplitSeen = 1;
            graph->Result->FidSplitParentLbn =
                directoryFile->Lbn;
        }
        graph->Result->FidsChecked++;
        if (!(attributes & 4) && (attributes & 8)) {
            if (++parents != 1 || fid[19] ||
                ((attributes & 2) != 0) ==
                    (directoryFile->StreamRole == 1) ||
                Read32(fid + 20) != UDFS_RECOVERY_SECTOR_SIZE ||
                Read32(fid + 24) != directoryFile->ParentLbn ||
                Read16(fid + 28) != directoryFile->ParentRef)
                return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        }
        if (!(attributes & (4 | 8))) {
            uint32_t encoded = Read32(fid + 20);
            uint32_t icbBytes = encoded & 0x3fffffff;
            uint32_t nameLength, hash, slot;
            uint8_t name[255];
            if (encoded >> 30 || !fid[19] ||
                (fid[38 + Read16(fid + 36)] != 8 &&
                 fid[38 + Read16(fid + 36)] != 16))
                return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
            status = RecoveryName(fid, name, &nameLength, &hash);
            if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
            slot = hash & (nameCapacity - 1);
            while (names[slot].OffsetPlusOne) {
                if (names[slot].Hash == hash) {
                    uint8_t other[255];
                    uint32_t otherLength, otherHash;
                    const uint8_t *previous = directory + names[slot].OffsetPlusOne - 1;
                    status = RecoveryName(previous, other, &otherLength, &otherHash);
                    if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
                    if (nameLength == otherLength &&
                        !RecoveryDifference(name, other, nameLength))
                        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
                }
                slot = (slot + 1) & (nameCapacity - 1);
            }
            names[slot].Hash = hash;
            names[slot].OffsetPlusOne = offset + 1;
            if (directoryFile->StreamRole == 1 && (attributes & 2))
                return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
            if (attributes & 2) subdirectories++;
            else if (directoryFile->StreamRole != 1)
                graph->FileReferences++;
            status = GraphPush(graph, Read16(fid + 28),
                                Read32(fid + 24), icbBytes,
                                (attributes & 2) != 0,
                                directoryFile->Ref, directoryFile->Lbn,
                                directoryFile->StreamRole == 1 ? 2 : 0,
                                directoryFile->StreamOwnerUniqueId);
            if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
        }
        offset += recordLength;
    }
    if (offset != length || parents != 1)
        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    /* Stream objects are owned by the graph but omitted from UDFS's LVID
     * file count.  The stream directory's parent points to the base FE. */
    if (directoryFile->StreamRole == 1)
        return subdirectories || links ? GRAPH_FAIL(UdfsRecoveryProbeCorrupt) :
               UdfsRecoveryProbeValidLayout;
    if (graph->Result->MirrorDirectorySplitCount &&
        directoryFile->Ref == 1 &&
        directoryFile->Lbn ==
            graph->Result->MirrorDirectoryLbn) {
        uint32_t expected = subdirectories + 1;
        if (graph->Result->DirectoryLinkFixCount ||
            graph->Result->MirrorDirectoryMainLinks != links ||
            (expected != links &&
             expected !=
                graph->Result->MirrorDirectoryMirrorLinks))
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        graph->Result->DirectoryLinkFixCount = 1;
        graph->Result->DirectoryLinkFixParentRef = directoryFile->Ref;
        graph->Result->DirectoryLinkFixParentLbn = directoryFile->Lbn;
        graph->Result->DirectoryLinkFixChildLbn = UINT32_MAX;
        graph->Result->DirectoryLinkFixStored = links;
        graph->Result->DirectoryLinkFixExpected = (uint16_t)expected;
        return UdfsRecoveryProbeValidLayout;
    }
    if (subdirectories + 1 == links)
        return UdfsRecoveryProbeValidLayout;
    /* Only the pre-FID side of one empty-directory deletion can have a
     * parent count one low.  Its still-reachable child FE must already be
     * pre-freed on disk.  All other count mismatches remain corruption. */
    if (subdirectories + 1 != (uint32_t)links + 1 ||
        graph->Result->DirectoryLinkFixCount ||
        directoryFile->Ref != 1)
        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    {
        uint32_t child = UINT32_MAX, count = 0, cursor = 0;
        while (cursor < length) {
            const uint8_t *fid = directory + cursor;
            uint32_t size = (38 + Read16(fid + 36) + fid[19] + 3) & ~3U;
            if (!(fid[18] & (4 | 8)) && (fid[18] & 2)) {
                int freeBit = 0;
                UDFS_RECOVERY_PROBE_STATUS status;
                if (Read16(fid + 28) != 1)
                    return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
                status = GraphMetadataBitIsFree(graph, Read32(fid + 24),
                                                &freeBit);
                if (status != UdfsRecoveryProbeValidLayout)
                    return GRAPH_FAIL(status);
                if (freeBit) {
                    if (++count > 1)
                        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
                    child = Read32(fid + 24);
                }
            }
            cursor += size;
        }
        if (count != 1)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        graph->Result->DirectoryLinkFixCount = 1;
        graph->Result->DirectoryLinkFixParentRef = directoryFile->Ref;
        graph->Result->DirectoryLinkFixParentLbn = directoryFile->Lbn;
        graph->Result->DirectoryLinkFixChildLbn = child;
        graph->Result->DirectoryLinkFixStored = links;
        graph->Result->DirectoryLinkFixExpected = (uint16_t)(subdirectories + 1);
    }
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
GraphProcessFile(RECOVERY_GRAPH *graph, const RECOVERY_GRAPH_ENTRY *file)
{
    uint8_t fe[UDFS_RECOVERY_SECTOR_SIZE];
    RECOVERY_EXTENT_LIST *dataExtents = graph->ScratchExtents;
    UDFS_RECOVERY_PROBE_STATUS status;
    uint32_t inlineStart = 0, inlineLength = 0, mode = 0, dirLength = 0;
    uint16_t kind;
    uint16_t links, flags;
    uint32_t streamIcbBytes = 0;
    uint64_t uniqueId;
    uint8_t *directory = NULL;
    RECOVERY_NAME_SLOT *names = NULL;
    int alreadyVisited;

    status = GraphVisit(graph, file, &alreadyVisited);
    if (status != UdfsRecoveryProbeValidLayout || alreadyVisited)
        return GRAPH_FAIL(status);
    status = GraphReadLogical(graph, file->Ref, file->Lbn, fe);
    if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
    kind = Read16(fe);
    if (kind != 261 && kind != 266)
        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    status = ValidateTag(fe, kind, file->Lbn);
    if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
    if (fe[27] != (file->StreamRole == 1 ? 13 :
                   (file->Directory ? 4 : 5)))
        return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    flags = Read16(fe + 34);
    links = Read16(fe + 48);
    if (kind == 266) streamIcbBytes = Read32(fe + 152);
    if (file->StreamRole == 1) {
        if (links || streamIcbBytes || (flags & 0x2000))
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    } else if (file->StreamRole == 2) {
        if (links != 1 || streamIcbBytes || !(flags & 0x2000))
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    } else {
        if (!links || (flags & 0x2000))
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        if (streamIcbBytes) {
            if (file->Directory || kind != 266 ||
                streamIcbBytes != UDFS_RECOVERY_SECTOR_SIZE || links != 2)
                return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
        } else if (!file->Directory && links != 1) {
            /* Ordinary hard links still require per-link accounting. */
            return GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
        }
    }
    uniqueId = Read64(fe + (kind == 266 ? 200 : 160));
    if (file->StreamRole) {
        /* UDFS stores the base FE's ID in its stream directory and all its
         * named streams.  Reject unbound or mismatched references. */
        if (!file->StreamOwnerUniqueId ||
            uniqueId != file->StreamOwnerUniqueId)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
    } else {
        status = GraphRecordUniqueId(graph, uniqueId, file->Directory &&
            file->Ref == file->ParentRef && file->Lbn == file->ParentLbn);
        if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
    }
    if (uniqueId > graph->Result->MaxUniqueId)
        graph->Result->MaxUniqueId = uniqueId;
    status = GraphOwn(graph, file->Ref, file->Lbn, file->Bytes);
    if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
    status = GraphParseFileAds(graph, file, fe, dataExtents,
                               &inlineStart, &inlineLength, &mode);
    if (status != UdfsRecoveryProbeValidLayout)
        return GRAPH_FAIL(status);
    if (streamIcbBytes) {
        status = GraphPush(graph, Read16(fe + 160), Read32(fe + 156),
                           streamIcbBytes, 0, file->Ref, file->Lbn,
                           1, uniqueId);
        if (status != UdfsRecoveryProbeValidLayout) return GRAPH_FAIL(status);
    }
    if (file->Directory && graph->Result->DirectoryLinkFixCount &&
        file->Ref == 1 &&
        file->Lbn == graph->Result->DirectoryLinkFixChildLbn) {
        if (file->ParentRef !=
                graph->Result->DirectoryLinkFixParentRef ||
            file->ParentLbn !=
                graph->Result->DirectoryLinkFixParentLbn ||
            mode != 3 || dataExtents->FileBytes != 40 ||
            Read16(fe + 48) != 1)
            return GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        graph->Result->DirectoryLinkFixChildVerified = 1;
    }
    if (!file->Directory && file->Ref == 1 && mode == 3)
        SetBit(graph->MetadataInlineFileFes, file->Lbn);
    if (!file->Directory && file->StreamRole != 1)
        return GRAPH_FAIL(status);
    status = GraphReadDirectory(graph, file, fe, dataExtents,
                                inlineStart, mode, &directory, &dirLength);
    if (status == UdfsRecoveryProbeValidLayout) {
        uint32_t capacity = 1;
        while (capacity < 2 * (dirLength / 40 + 1)) capacity <<= 1;
        names = (RECOVERY_NAME_SLOT *)graph->Reader->Allocate(
            graph->Reader->Context, (size_t)capacity * sizeof(*names));
        if (!names) status = UdfsRecoveryProbeNoMemory;
        else {
            memset(names, 0, (size_t)capacity * sizeof(*names));
            status = GraphScanFids(graph, file, dataExtents, mode,
                                   directory, dirLength, Read16(fe + 48),
                                   names, capacity);
        }
    }
    if (names)
        graph->Reader->Free(graph->Reader->Context, names);
    if (directory)
        graph->Reader->Free(graph->Reader->Context, directory);
    return GRAPH_FAIL(status);
}

/* Collect only free-to-owned bitmap transitions justified by the complete
 * namespace walk. The heap-allocated plan bounds distinct sectors, so a batch
 * of creates need not fit a guessed one-file crash pattern. */
static UDFS_RECOVERY_PROBE_STATUS
PlanOwnedMetadataBlock(const UDFS_RECOVERY_READER *reader,
                       UDFS_RECOVERY_PROBE *result,
                       const RECOVERY_EXTENT_LIST *bitmapFile,
                       UDFS_RECOVERY_REPAIR_PLAN *plan, uint32_t lbn)
{
    uint32_t lba, offset, i;
    uint8_t mask = (uint8_t)(1U << (lbn & 7));
    UDFS_RECOVERY_SECTOR_EDIT *edit;
    UDFS_RECOVERY_PROBE_STATUS status;

    status = MapExtentByte(result, bitmapFile, 24ULL + (lbn >> 3),
                            &lba, &offset);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    for (i = 0; i < plan->BitmapEditCount; ++i)
        if (plan->BitmapEdits[i].Lba == lba) break;
    if (i == plan->BitmapEditCount) {
        if (i == UDFS_RECOVERY_MAX_BITMAP_EDITS)
            return UdfsRecoveryProbeUnsupported;
        edit = &plan->BitmapEdits[i];
        edit->Lba = lba;
        status = ReadSector(reader, result, lba, edit->Before);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        memcpy(edit->After, edit->Before, UDFS_RECOVERY_SECTOR_SIZE);
        plan->BitmapEditCount++;
    }
    edit = &plan->BitmapEdits[i];
    if (!(edit->After[offset] & mask)) return UdfsRecoveryProbeCorrupt;
    edit->After[offset] &= (uint8_t)~mask;
    /* Some valid small SBDs cover bitmap bytes in their descriptor CRC.
     * Preserve that coverage instead of assuming the usual eight-byte CRC. */
    {
        uint32_t headerLba, headerOffset;
        status = MapExtentByte(result, bitmapFile, 0, &headerLba, &headerOffset);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (lba == headerLba) {
            uint8_t *tag = edit->After + headerOffset;
            uint32_t length = Read16(tag + 10), sum = 0, j;
            uint16_t crc;
            if ((!bitmapFile->Inline && headerOffset) ||
                headerOffset > UDFS_RECOVERY_SECTOR_SIZE - 24 ||
                (bitmapFile->Inline && bitmapFile->FileBytes >
                    UDFS_RECOVERY_SECTOR_SIZE - headerOffset) ||
                length > (bitmapFile->Inline ?
                    bitmapFile->FileBytes - 16 :
                    UDFS_RECOVERY_SECTOR_SIZE - 16))
                return UdfsRecoveryProbeCorrupt;
            crc = TagCrc(tag + 16, length);
            tag[8] = (uint8_t)crc;
            tag[9] = (uint8_t)(crc >> 8);
            for (j = 0; j < 16; ++j)
                if (j != 4) sum += tag[j];
            tag[4] = (uint8_t)sum;
            if (bitmapFile->Inline) {
                /* The embedded SBD lives inside an EFE. Its metadata bits
                 * are covered by the enclosing EFE descriptor CRC even when
                 * the SBD itself covers only its eight-byte size header. */
                length = Read16(edit->After + 10);
                if (length > UDFS_RECOVERY_SECTOR_SIZE - 16 ||
                    length < headerOffset + bitmapFile->FileBytes - 16)
                    return UdfsRecoveryProbeCorrupt;
                crc = TagCrc(edit->After + 16, length);
                edit->After[8] = (uint8_t)crc;
                edit->After[9] = (uint8_t)(crc >> 8);
                sum = 0;
                for (j = 0; j < 16; ++j)
                    if (j != 4) sum += edit->After[j];
                edit->After[4] = (uint8_t)sum;
                status = ValidateTag(edit->After, 266,
                                     bitmapFile->InlineFeLbn);
                if (status != UdfsRecoveryProbeValidLayout)
                    return status;
            }
        }
    }
    return UdfsRecoveryProbeValidLayout;
}

/* Plan one physical bitmap transition. Free-to-owned is justified by the
 * namespace graph; owned-to-free is used only by the separately checked
 * uncommitted-allocation rollback gate. */
static UDFS_RECOVERY_PROBE_STATUS
PlanPhysicalBitmapBlock(const UDFS_RECOVERY_READER *reader,
                        UDFS_RECOVERY_PROBE *result,
                        UDFS_RECOVERY_REPAIR_PLAN *plan, uint32_t lbn,
                        int makeFree)
{
    uint64_t byteOffset = 24ULL + (lbn >> 3);
    uint64_t address = (uint64_t)result->PartitionStart +
                       result->PhysicalBitmapLbn +
                       byteOffset / UDFS_RECOVERY_SECTOR_SIZE;
    uint32_t lba, offset, i;
    uint8_t mask = (uint8_t)(1U << (lbn & 7));
    UDFS_RECOVERY_SECTOR_EDIT *edit;
    UDFS_RECOVERY_PROBE_STATUS status;

    if (address >= reader->SectorCount) return UdfsRecoveryProbeCorrupt;
    lba = (uint32_t)address;
    offset = (uint32_t)(byteOffset % UDFS_RECOVERY_SECTOR_SIZE);
    for (i = 0; i < plan->BitmapEditCount; ++i)
        if (plan->BitmapEdits[i].Lba == lba) break;
    if (i == plan->BitmapEditCount) {
        if (i == UDFS_RECOVERY_MAX_BITMAP_EDITS)
            return UdfsRecoveryProbeUnsupported;
        edit = &plan->BitmapEdits[i];
        edit->Lba = lba;
        status = ReadSector(reader, result, lba, edit->Before);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        memcpy(edit->After, edit->Before, UDFS_RECOVERY_SECTOR_SIZE);
        plan->BitmapEditCount++;
    }
    edit = &plan->BitmapEdits[i];
    if (!!(edit->After[offset] & mask) == !!makeFree)
        return UdfsRecoveryProbeCorrupt;
    if (makeFree)
        edit->After[offset] |= mask;
    else
        edit->After[offset] &= (uint8_t)~mask;
    if (lba == result->PartitionStart + result->PhysicalBitmapLbn) {
        uint32_t length = Read16(edit->After + 10), sum = 0, j;
        uint16_t crc;
        if (length > UDFS_RECOVERY_SECTOR_SIZE - 16)
            return UdfsRecoveryProbeCorrupt;
        crc = TagCrc(edit->After + 16, length);
        edit->After[8] = (uint8_t)crc;
        edit->After[9] = (uint8_t)(crc >> 8);
        for (j = 0; j < 16; ++j)
            if (j != 4) sum += edit->After[j];
        edit->After[4] = (uint8_t)sum;
    }
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
PlanParentLinkCount(const UDFS_RECOVERY_READER *reader,
                    UDFS_RECOVERY_PROBE *result,
                    const RECOVERY_EXTENT_LIST *mainFile,
                    const RECOVERY_EXTENT_LIST *mirrorFile,
                    UDFS_RECOVERY_REPAIR_PLAN *plan)
{
    uint32_t lba[2], i;
    UDFS_RECOVERY_PROBE_STATUS status;
    if (!result->MetadataDuplicated ||
        result->DirectoryLinkFixParentRef != 1 ||
        result->DirectoryLinkFixCount != 1 ||
        plan->FeEditCount)
        return UdfsRecoveryProbeCorrupt;
    status = MapMetadataBlock(result, mainFile,
                 result->DirectoryLinkFixParentLbn, &lba[0]);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    status = MapMetadataBlock(result, mirrorFile,
                 result->DirectoryLinkFixParentLbn, &lba[1]);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    if (lba[0] == lba[1]) return UdfsRecoveryProbeCorrupt;
    for (i = 0; i < 2; ++i) {
        UDFS_RECOVERY_SECTOR_EDIT *edit = &plan->FeEdits[i];
        uint16_t kind, crcLength, crc;
        uint32_t checksum = 0, j;
        edit->Lba = lba[i];
        status = ReadSector(reader, result, edit->Lba, edit->Before);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        kind = Read16(edit->Before);
        if ((kind != 261 && kind != 266) ||
            edit->Before[27] != 4 ||
            (i == 0 && Read16(edit->Before + 48) !=
                result->DirectoryLinkFixStored) ||
            (result->MirrorDirectorySplitCount &&
             Read16(edit->Before + 48) !=
                (i == 0 ? result->MirrorDirectoryMainLinks :
                          result->MirrorDirectoryMirrorLinks)))
            return UdfsRecoveryProbeCorrupt;
        status = ValidateTag(edit->Before, kind,
                             result->DirectoryLinkFixParentLbn);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        crcLength = Read16(edit->Before + 10);
        if (crcLength < 50 - 16 ||
            crcLength > UDFS_RECOVERY_SECTOR_SIZE - 16)
            return UdfsRecoveryProbeCorrupt;
        memcpy(edit->After, edit->Before, UDFS_RECOVERY_SECTOR_SIZE);
        Write16(edit->After + 48,
                result->DirectoryLinkFixExpected);
        crc = TagCrc(edit->After + 16, crcLength);
        Write16(edit->After + 8, crc);
        edit->After[4] = 0;
        for (j = 0; j < 16; ++j)
            if (j != 4) checksum += edit->After[j];
        edit->After[4] = (uint8_t)checksum;
        status = ValidateTag(edit->After, kind,
                             result->DirectoryLinkFixParentLbn);
        if (status != UdfsRecoveryProbeValidLayout) return status;
    }
    if (result->MirrorDirectorySplitCount) {
        if (result->MirrorDirectoryLbn !=
                result->DirectoryLinkFixParentLbn ||
            !DirectoryFePairDiffersOnlyInLinkCount(
                plan->FeEdits[0].Before,
                plan->FeEdits[1].Before,
                result->DirectoryLinkFixParentLbn))
            return UdfsRecoveryProbeCorrupt;
    } else if (RecoveryDifference(plan->FeEdits[0].Before,
                                  plan->FeEdits[1].Before,
                                  UDFS_RECOVERY_SECTOR_SIZE) != 0)
        return UdfsRecoveryProbeCorrupt;
    if (Read16(plan->FeEdits[0].Before + 48) !=
            result->DirectoryLinkFixExpected &&
        Read16(plan->FeEdits[1].Before + 48) !=
            result->DirectoryLinkFixExpected &&
        result->MirrorDirectorySplitCount)
        return UdfsRecoveryProbeCorrupt;
    if (RecoveryDifference(plan->FeEdits[0].Before,
                           plan->FeEdits[0].After,
                           UDFS_RECOVERY_SECTOR_SIZE) != 0) {
        plan->FeEditCount = 1;
    }
    if (RecoveryDifference(plan->FeEdits[1].Before,
                           plan->FeEdits[1].After,
                           UDFS_RECOVERY_SECTOR_SIZE) != 0) {
        if (plan->FeEditCount)
            plan->FeEditCount = 2;
        else {
            plan->FeEdits[0] = plan->FeEdits[1];
            plan->FeEditCount = 1;
        }
    }
    if (!plan->FeEditCount)
        return UdfsRecoveryProbeCorrupt;
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
PlanLiveFidCopy(const UDFS_RECOVERY_READER *reader,
                UDFS_RECOVERY_PROBE *result,
                UDFS_RECOVERY_REPAIR_PLAN *plan)
{
    uint8_t mainSector[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t mirror[UDFS_RECOVERY_SECTOR_SIZE];
    const uint8_t *live, *deleted;
    int liveOnMirror;
    uint32_t child, offset;
    UDFS_RECOVERY_PROBE_STATUS status;
    if (result->FidSplitCount != 1 ||
        result->MirrorDirectorySplitCount ||
        !result->FidSplitSeen || plan->FidEditCount)
        return UdfsRecoveryProbeCorrupt;
    status = ReadSector(reader, result, result->FidSplitMainLba, mainSector);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    status = ReadSector(reader, result, result->FidSplitMirrorLba,
                        mirror);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    if (!DirectoryFidPairLiveDeleted(mainSector, mirror,
            result->FidSplitLbn, &liveOnMirror, &child, &offset) ||
        (uint8_t)liveOnMirror != result->FidSplitLiveOnMirror ||
        child != result->FidSplitChildLbn ||
        offset != result->FidSplitRecordOffset)
        return UdfsRecoveryProbeCorrupt;
    live = liveOnMirror ? mirror : mainSector;
    deleted = liveOnMirror ? mainSector : mirror;
    plan->FidEdit.Lba = liveOnMirror ?
        result->FidSplitMainLba : result->FidSplitMirrorLba;
    plan->FidLiveLba = liveOnMirror ?
        result->FidSplitMirrorLba : result->FidSplitMainLba;
    plan->FidEditLogicalLbn = result->FidSplitLbn;
    memcpy(plan->FidEdit.Before, deleted,
           UDFS_RECOVERY_SECTOR_SIZE);
    memcpy(plan->FidEdit.After, live,
           UDFS_RECOVERY_SECTOR_SIZE);
    plan->FidEditCount = 1;
    return UdfsRecoveryProbeValidLayout;
}

/* A bitmap-free metadata block can still contain a recently written FE
 * whose bitmap bit and parent FID did not survive the cut. Refuse physical
 * rollback if either copy holds a plausible nonempty FE there: its data may
 * be the otherwise unowned physical allocation. Empty FEs carry no AD and
 * cannot own that allocation. Check all free slots, not only the first one. */
static UDFS_RECOVERY_PROBE_STATUS
GraphCheckFreeMetadataFes(RECOVERY_GRAPH *graph,
                          const RECOVERY_EXTENT_LIST *mirrorFile,
                          uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE])
{
    uint32_t lbn, copy;
    UDFS_RECOVERY_PROBE *result = graph->Result;
    UDFS_RECOVERY_PROBE_STATUS status;
    if (!result->MetadataDuplicated)
        return UdfsRecoveryProbeUnsupported;
    for (lbn = 0; lbn < result->MetadataBlocks; ++lbn) {
        int freeBit;
        status = GraphMetadataBitIsFree(graph, lbn, &freeBit);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (!freeBit) continue;
        for (copy = 0; copy < 2; ++copy) {
            uint32_t lba, tag, adOffset;
            const RECOVERY_EXTENT_LIST *file =
                copy ? mirrorFile : graph->Metadata;
            status = MapMetadataBlock(result, file, lbn, &lba);
            if (status != UdfsRecoveryProbeValidLayout) return status;
            status = ReadSector(graph->Reader, result, lba, sector);
            if (status != UdfsRecoveryProbeValidLayout) return status;
            tag = Read16(sector);
            if (tag != 261 && tag != 266) continue;
            /* An FE-like sector with a broken tag cannot prove that it has
             * no live allocation descriptors. Keep the volume read-only. */
            if (ValidateTag(sector, (uint16_t)tag, lbn) !=
                UdfsRecoveryProbeValidLayout)
                return UdfsRecoveryProbeUnsupported;
            adOffset = tag == 266 ? 212 : 172;
            if (Read64(sector + 56) || Read32(sector + adOffset))
                return UdfsRecoveryProbeUnsupported;
        }
    }
    result->FreeMetadataFeScanComplete = 1;
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
GraphRun(const UDFS_RECOVERY_READER *reader, UDFS_RECOVERY_PROBE *result,
         const RECOVERY_EXTENT_LIST *mainFile,
         const RECOVERY_EXTENT_LIST *mirrorFile,
         const RECOVERY_EXTENT_LIST *bitmapFile,
         const uint8_t lvd[UDFS_RECOVERY_SECTOR_SIZE],
         uint32_t bitmapExtentBytes, UDFS_RECOVERY_REPAIR_PLAN *plan,
         uint8_t fsd[UDFS_RECOVERY_SECTOR_SIZE])
{
    RECOVERY_GRAPH *graph;
    UDFS_RECOVERY_PROBE_STATUS status = UdfsRecoveryProbeValidLayout;
    uint32_t metadataBytes, i;
    uint32_t fsdEncoded, rootEncoded;
    uint16_t fsdRef;

    /* The mount thread has a 12 KiB kernel stack and will issue lower-device
     * I/O while this graph is live.  Keep its sector caches off that stack. */
    graph = (RECOVERY_GRAPH *)reader->Allocate(reader->Context,
                                               sizeof(*graph));
    if (!graph) return UdfsRecoveryProbeNoMemory;
    memset(graph, 0, sizeof(*graph));
    graph->Reader = reader;
    graph->Result = result;
    graph->Metadata = mainFile;
    graph->BitmapFile = bitmapFile;
    graph->ScratchExtents = (RECOVERY_EXTENT_LIST *)reader->Allocate(
        reader->Context, sizeof(*graph->ScratchExtents) +
        (size_t)UDFS_RECOVERY_MAX_DIR_EXTENTS * sizeof(RECOVERY_EXTENT));
    if (graph->ScratchExtents) {
        memset(graph->ScratchExtents, 0, sizeof(*graph->ScratchExtents));
        graph->ScratchExtents->Items =
            (RECOVERY_EXTENT *)(graph->ScratchExtents + 1);
        graph->ScratchExtents->Capacity = UDFS_RECOVERY_MAX_DIR_EXTENTS;
    }
    metadataBytes = (uint32_t)(((uint64_t)result->MetadataBlocks + 7) / 8);
    status = BitmapInitialize(reader, &graph->PhysicalOwned,
                               result->PartitionBlocks);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = BitmapInitialize(reader, &graph->PhysicalVisited,
                               result->PartitionBlocks);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = BitmapInitialize(reader, &graph->PhysicalDirs,
                               result->PartitionBlocks);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    graph->MetadataOwned = (uint8_t *)reader->Allocate(reader->Context,
                                                       metadataBytes);
    graph->MetadataVisited = (uint8_t *)reader->Allocate(reader->Context,
                                                         metadataBytes);
    graph->MetadataDirs = (uint8_t *)reader->Allocate(reader->Context,
                                                      metadataBytes);
    graph->MetadataInlineFileFes = (uint8_t *)reader->Allocate(
        reader->Context, metadataBytes);
    if (!graph->ScratchExtents || !graph->MetadataOwned ||
        !graph->MetadataVisited || !graph->MetadataDirs ||
        !graph->MetadataInlineFileFes) {
        status = UdfsRecoveryProbeNoMemory;
        goto done;
    }
    memset(graph->MetadataOwned, 0, metadataBytes);
    memset(graph->MetadataVisited, 0, metadataBytes);
    memset(graph->MetadataDirs, 0, metadataBytes);
    memset(graph->MetadataInlineFileFes, 0, metadataBytes);

    for (i = 0; i < mainFile->Count; ++i) {
        status = GraphOwn(graph, 0, mainFile->Items[i].Lbn,
                           mainFile->Items[i].Bytes);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
    }
    if (result->MetadataDuplicated)
        for (i = 0; i < mirrorFile->Count; ++i) {
            status = GraphOwn(graph, 0, mirrorFile->Items[i].Lbn,
                               mirrorFile->Items[i].Bytes);
            if (status != UdfsRecoveryProbeValidLayout) goto done;
        }
    status = GraphOwn(graph, 0, result->PhysicalBitmapLbn,
                       bitmapExtentBytes);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = GraphOwn(graph, 0, result->MetadataMainFeLbn,
                       UDFS_RECOVERY_SECTOR_SIZE);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = GraphOwn(graph, 0, result->MetadataMirrorFeLbn,
                       UDFS_RECOVERY_SECTOR_SIZE);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = GraphOwn(graph, 0, result->MetadataBitmapFeLbn,
                       UDFS_RECOVERY_SECTOR_SIZE);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    for (i = 0; i < bitmapFile->Count; ++i) {
        status = GraphOwn(graph, 0, bitmapFile->Items[i].Lbn,
                           bitmapFile->Items[i].Bytes);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
    }

    fsdEncoded = Read32(lvd + 248);
    fsdRef = Read16(lvd + 256);
    if (fsdEncoded >> 30 ||
        (fsdEncoded & 0x3fffffff) != UDFS_RECOVERY_SECTOR_SIZE) {
        status = GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
        goto done;
    }
    status = GraphOwn(graph, fsdRef, Read32(lvd + 252),
                       UDFS_RECOVERY_SECTOR_SIZE);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = GraphReadLogical(graph, fsdRef, Read32(lvd + 252), fsd);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = ValidateTag(fsd, 256, Read32(lvd + 252));
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    if (Read16(fsd + 10) < 416 - 16) {
        status = GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        goto done;
    }
    rootEncoded = Read32(fsd + 400);
    if (rootEncoded >> 30 ||
        (rootEncoded & 0x3fffffff) != UDFS_RECOVERY_SECTOR_SIZE) {
        status = GRAPH_FAIL(UdfsRecoveryProbeUnsupported);
        goto done;
    }
    status = GraphPush(graph, Read16(fsd + 408),
                       Read32(fsd + 404), rootEncoded & 0x3fffffff, 1,
                       Read16(fsd + 408), Read32(fsd + 404), 0, 0);
    if (status != UdfsRecoveryProbeValidLayout) goto done;

    while (graph->QueueCount) {
        RECOVERY_GRAPH_ENTRY entry = graph->Queue[--graph->QueueCount];
        status = GraphProcessFile(graph, &entry);
        if (status != UdfsRecoveryProbeValidLayout) {
            result->FailureObjectRef = entry.Ref;
            result->FailureObjectLbn = entry.Lbn;
            goto done;
        }
    }

    if (graph->FileReferences != result->FilesVisited) {
        status = GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        goto done;
    }

    for (i = 0; i < (result->PartitionBlocks + 7ULL) / 8; ++i) {
        uint32_t byteOffset = 24 + i;
        uint32_t first = i * 8, remaining = result->PartitionBlocks - first;
        uint8_t value, owned, referencedFree, unowned;
        uint8_t mask = remaining < 8 ? (uint8_t)((1U << remaining) - 1) : 0xff;
        status = ReadCachedByte(reader, result, &graph->PhysicalBitmapCache,
                    result->PartitionStart + result->PhysicalBitmapLbn +
                    byteOffset / UDFS_RECOVERY_SECTOR_SIZE,
                    byteOffset % UDFS_RECOVERY_SECTOR_SIZE, &value);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        owned = BitmapReadByte(&graph->PhysicalOwned, i);
        referencedFree = value & owned & mask;
        unowned = (uint8_t)~(value | owned) & mask;
        if (referencedFree) {
            if (plan) {
                uint8_t remainingBits = referencedFree;
                while (remainingBits) {
                    uint32_t bit = FirstByteBit(remainingBits);
                    status = PlanPhysicalBitmapBlock(reader, result, plan,
                                                     first + bit, 0);
                    if (status != UdfsRecoveryProbeValidLayout) goto done;
                    remainingBits &= (uint8_t)(remainingBits - 1);
                }
            }
            if (!result->ReferencedFreePhysicalBlocks)
                result->FirstReferencedFreePhysicalLbn = first + FirstByteBit(referencedFree);
            result->ReferencedFreePhysicalBlocks += CountByteBits(referencedFree);
        }
        if (unowned) {
            if (plan) {
                uint8_t remainingBits = unowned;
                while (remainingBits) {
                    uint32_t bit = FirstByteBit(remainingBits);
                    status = PlanPhysicalBitmapBlock(reader, result, plan,
                                                     first + bit, 1);
                    if (status != UdfsRecoveryProbeValidLayout) goto done;
                    remainingBits &= (uint8_t)(remainingBits - 1);
                }
            }
            if (!result->UnownedPhysicalBlocks)
                result->FirstUnownedPhysicalLbn = first + FirstByteBit(unowned);
            result->UnownedPhysicalBlocks += CountByteBits(unowned);
        }
    }
    for (i = 0; i < result->MetadataBlocks; ++i) {
        uint32_t bitmapLba, bitmapOffset;
        uint8_t value;
        status = MapExtentByte(result, bitmapFile, 24ULL + (i >> 3),
                               &bitmapLba, &bitmapOffset);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        status = ReadCachedByte(reader, result, &graph->MetadataBitmapCache,
                                bitmapLba, bitmapOffset, &value);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        if ((value >> (i & 7)) & 1) {
            if (BitIsSet(graph->MetadataOwned, i)) {
                if (result->MetadataDuplicated) {
                    uint32_t mainLba, mirrorLba;
                    status = MapMetadataBlock(result, mainFile, i, &mainLba);
                    if (status != UdfsRecoveryProbeValidLayout) goto done;
                    status = MapMetadataBlock(result, mirrorFile, i, &mirrorLba);
                    if (status != UdfsRecoveryProbeValidLayout) goto done;
                    status = ReadSector(reader, result, mainLba, fsd);
                    if (status != UdfsRecoveryProbeValidLayout) goto done;
                    status = ReadSector(reader, result, mirrorLba,
                                        graph->PhysicalBitmapCache.Data);
                    if (status != UdfsRecoveryProbeValidLayout) goto done;
                    if (RecoveryDifference(fsd, graph->PhysicalBitmapCache.Data,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0) {
                        result->FailureLba = mainLba;
                        status = GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
                        goto done;
                    }
                    result->ReferencedFreeMirrorBlocksCompared++;
                }
                if (!result->ReferencedFreeMetadataBlocks)
                    result->FirstReferencedFreeMetadataLbn = i;
                if (BitIsSet(graph->MetadataInlineFileFes, i))
                    result->ReferencedFreeInlineFileFeBlocks++;
                if (result->ReferencedFreeMetadataBlocks <
                    UDFS_RECOVERY_MAX_REPAIR_BLOCKS)
                    result->ReferencedFreeMetadataLbn[
                        result->ReferencedFreeMetadataBlocks] = i;
                result->ReferencedFreeMetadataBlocks++;
                if (plan) {
                    status = PlanOwnedMetadataBlock(reader, result, bitmapFile,
                                                     plan, i);
                    if (status != UdfsRecoveryProbeValidLayout) goto done;
                }
            }
        } else if (!BitIsSet(graph->MetadataOwned, i)) {
            if (!result->UnownedMetadataBlocks)
                result->FirstUnownedMetadataLbn = i;
            result->UnownedMetadataBlocks++;
        }
    }
    if (result->FidSplitCount) {
        if (result->MirrorDirectorySplitCount ||
            !result->FidSplitSeen ||
            result->DirectoryLinkFixCount != 1 ||
            result->FidSplitParentLbn !=
                result->DirectoryLinkFixParentLbn ||
            result->FidSplitChildLbn !=
                result->DirectoryLinkFixChildLbn) {
            status = GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
            goto done;
        }
        if (plan) {
            status = PlanLiveFidCopy(reader, result, plan);
            if (status != UdfsRecoveryProbeValidLayout)
                goto done;
        }
    }
    if (result->MirrorDirectorySplitCount &&
        (!result->DirectoryLinkFixCount ||
         result->DirectoryLinkFixParentLbn !=
            result->MirrorDirectoryLbn)) {
        status = GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
        goto done;
    }
    if (result->DirectoryLinkFixCount) {
        if (result->UnownedPhysicalBlocks ||
            result->UnownedMetadataBlocks || result->OverlapBlocks ||
            (!result->MirrorDirectorySplitCount &&
             (!result->DirectoryLinkFixChildVerified ||
              result->ReferencedFreeMetadataBlocks != 1 ||
              result->FirstReferencedFreeMetadataLbn !=
                  result->DirectoryLinkFixChildLbn ||
              result->ReferencedFreePhysicalBlocks))) {
            status = GRAPH_FAIL(UdfsRecoveryProbeCorrupt);
            goto done;
        }
        if (plan) {
            status = PlanParentLinkCount(reader, result, mainFile,
                                         mirrorFile, plan);
            if (status != UdfsRecoveryProbeValidLayout)
                goto done;
        }
    }
    if (result->UnownedPhysicalBlocks && !result->UnownedMetadataBlocks &&
        !result->ReferencedFreePhysicalBlocks &&
        !result->ReferencedFreeMetadataBlocks && !result->OverlapBlocks) {
        status = GraphCheckFreeMetadataFes(graph, mirrorFile, fsd);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
    }
    result->OwnershipGraphScanned = 1;
    if (result->OverlapBlocks || result->ReferencedFreePhysicalBlocks ||
        result->ReferencedFreeMetadataBlocks || result->UnownedPhysicalBlocks ||
        result->UnownedMetadataBlocks)
        status = UdfsRecoveryProbeGraphMismatch;
    else
        result->CompleteOwnershipGraph = 1;
done:
    if (graph->ScratchExtents) reader->Free(reader->Context, graph->ScratchExtents);
    if (graph->Queue) reader->Free(reader->Context, graph->Queue);
    if (graph->UniqueIds) reader->Free(reader->Context, graph->UniqueIds);
    BitmapRelease(reader, &graph->PhysicalOwned);
    if (graph->MetadataOwned) reader->Free(reader->Context, graph->MetadataOwned);
    BitmapRelease(reader, &graph->PhysicalVisited);
    if (graph->MetadataVisited) reader->Free(reader->Context, graph->MetadataVisited);
    BitmapRelease(reader, &graph->PhysicalDirs);
    if (graph->MetadataDirs) reader->Free(reader->Context, graph->MetadataDirs);
    if (graph->MetadataInlineFileFes)
        reader->Free(reader->Context, graph->MetadataInlineFileFes);
    reader->Free(reader->Context, graph);
    return status;
}

typedef struct _RECOVERY_LAYOUT_WORK {
    uint8_t Sector[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Pd[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Lvd[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t MainSector[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t MirrorSector[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t PrevailingLvid[UDFS_RECOVERY_SECTOR_SIZE];
    RECOVERY_SECTOR_CACHE PhysicalCache;
    RECOVERY_SECTOR_CACHE MetadataCache;
    RECOVERY_EXTENT_LIST Lists[3];
    RECOVERY_EXTENT ListItems[3][UDFS_RECOVERY_MAX_EXTENTS];
} RECOVERY_LAYOUT_WORK;

static void
RecoveryAnchorCandidates(uint32_t sectorCount, uint32_t anchors[8])
{
    uint32_t last = sectorCount - 1;
    anchors[0] = 256;
    anchors[1] = 512;
    anchors[2] = last - 256;
    anchors[3] = last - 255;
    anchors[4] = last - 258;
    anchors[5] = last - 2;
    anchors[6] = last;
    anchors[7] = last - 512;
}

/* Only the single-session disk class is supported. Keep the ordinary mount's
 * anchor order (duplicate session/track candidates omitted), and never select
 * another anchor merely because an accepted anchor's ownership graph fails. */
static UDFS_RECOVERY_PROBE_STATUS
ReadRecoveryDescriptors(const UDFS_RECOVERY_READER *reader,
                        UDFS_RECOVERY_PROBE *result, uint32_t start,
                        uint32_t bytes, uint8_t *sector,
                        uint8_t *pd, uint8_t *lvd)
{
    UDFS_RECOVERY_PROBE_STATUS status;
    uint32_t i, count;
    int havePd = 0, haveLvd = 0, havePvd = 0;

    if (!bytes || (bytes & (UDFS_RECOVERY_SECTOR_SIZE - 1)))
        return UdfsRecoveryProbeCorrupt;
    if (bytes > 256 * UDFS_RECOVERY_SECTOR_SIZE)
        return UdfsRecoveryProbeUnsupported;
    count = bytes / UDFS_RECOVERY_SECTOR_SIZE;
    if (start >= reader->SectorCount || count > reader->SectorCount - start)
        return UdfsRecoveryProbeCorrupt;
    for (i = 0; i < count; ++i) {
        uint16_t kind;
        result->FailureLba = start + i;
        status = ReadSector(reader, result, start + i, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        kind = Read16(sector);
        status = ValidateTag(sector, kind, start + i);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        switch (kind) {
        case 1:
            if (havePvd) return UdfsRecoveryProbeUnsupported;
            havePvd = 1;
            break;
        case 4:
        case 7:
            break;
        case 5:
            if (havePd) return UdfsRecoveryProbeUnsupported;
            if (Read16(sector + 10) < 196 - 16)
                return UdfsRecoveryProbeCorrupt;
            memcpy(pd, sector, UDFS_RECOVERY_SECTOR_SIZE);
            havePd = 1;
            break;
        case 6:
            if (haveLvd) return UdfsRecoveryProbeUnsupported;
            /* Include the complete supported partition map in the CRC. */
            if (Read16(sector + 10) < 510 - 16)
                return UdfsRecoveryProbeCorrupt;
            memcpy(lvd, sector, UDFS_RECOVERY_SECTOR_SIZE);
            haveLvd = 1;
            break;
        case 8:
            if (!havePd || !haveLvd || !havePvd)
                return UdfsRecoveryProbeCorrupt;
            result->VdsStartLba = start;
            result->FailureLba = 0;
            return UdfsRecoveryProbeValidLayout;
        default:
            /* In particular, do not silently ignore a VDS continuation and
             * validate a different descriptor set from the ordinary mount. */
            return UdfsRecoveryProbeUnsupported;
        }
    }
    return UdfsRecoveryProbeCorrupt;
}

static UDFS_RECOVERY_PROBE_STATUS
SelectRecoveryDescriptors(const UDFS_RECOVERY_READER *reader,
                          UDFS_RECOVERY_PROBE *result, uint8_t *sector,
                          uint8_t *pd, uint8_t *lvd)
{
    UDFS_RECOVERY_PROBE_STATUS status = UdfsRecoveryProbeCorrupt;
    uint32_t anchors[8];
    uint32_t i, mainStart, mainBytes, reserveStart, reserveBytes;

    RecoveryAnchorCandidates(reader->SectorCount, anchors);
    for (i = 0; i < 8; ++i) {
        if (!anchors[i] || anchors[i] >= reader->SectorCount) continue;
        result->FailureLba = anchors[i];
        status = ReadSector(reader, result, anchors[i], sector);
        if (status == UdfsRecoveryProbeValidLayout)
            status = ValidateTag(sector, 2, anchors[i]);
        if (status == UdfsRecoveryProbeUnsupported) return status;
        if (status != UdfsRecoveryProbeValidLayout) continue;
        result->AnchorLba = anchors[i];
        /* A valid earlier anchor remains authoritative, even if its VDS is
         * unusable. Falling through to a later one could recover a stale view. */
        if (Read16(sector + 10) < 32 - 16)
            return UdfsRecoveryProbeCorrupt;
        mainBytes = Read32(sector + 16);
        mainStart = Read32(sector + 20);
        reserveBytes = Read32(sector + 24);
        reserveStart = Read32(sector + 28);
        result->MainVdsStartLba = mainStart;
        result->MainVdsBytes = mainBytes;
        result->ReserveVdsStartLba = reserveStart;
        result->ReserveVdsBytes = reserveBytes;
        status = ReadRecoveryDescriptors(reader, result, mainStart, mainBytes,
                                          sector, pd, lvd);
        if (status != UdfsRecoveryProbeCorrupt &&
            status != UdfsRecoveryProbeIoError)
            return status;
        return ReadRecoveryDescriptors(reader, result, reserveStart,
                                         reserveBytes, sector, pd, lvd);
    }
    return status;
}

/* Keep transient sector snapshots off the mount thread's limited kernel
 * stack. The caller's allocator supplies nonpaged memory in the driver. */
typedef struct _RECOVERY_INTEGRITY_WORK {
    uint8_t PreviousClosed[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t JournalOpen[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t OlderJournal[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Expected[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Terminator[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Candidate[UDFS_RECOVERY_SECTOR_SIZE];
} RECOVERY_INTEGRITY_WORK;

/* Match UDFLoadLogicalVolInt's prevailing descriptor walk: an LVID with a
 * nextIntegrityExt jumps to that extent before the remaining sectors of the
 * current extent. The earlier CLOSED descriptor must not hide a later OPEN
 * checkpoint. Bound both each extent and the total walk before any repair. */
static UDFS_RECOVERY_PROBE_STATUS
ReadPrevailingIntegrityWorker(const UDFS_RECOVERY_READER *reader,
                              UDFS_RECOVERY_PROBE *result,
                              uint32_t extentBytes, uint32_t extentLba,
                              uint8_t *sector, uint8_t *prevailing,
                              uint32_t *prevailingLba,
                              uint8_t *integrityType,
                              uint8_t *checksumRepair,
                              RECOVERY_INTEGRITY_WORK *work)
{
    uint32_t visited = 0;
    uint32_t anchors[8];
    uint8_t lastType = 0xff;
    uint16_t previousVersion = 0, previousSerial = 0;
    uint16_t previousCrcLength = 0;
    uint32_t previousClosedLba = 0;
    uint8_t *previousClosed = work->PreviousClosed;
    uint8_t *journalOpen = work->JournalOpen;
    uint8_t *olderJournal = work->OlderJournal;
    uint8_t *expected = work->Expected;
    uint8_t *terminator = work->Terminator;
    uint8_t *candidate = work->Candidate;
    uint32_t journalOpenLba = 0, journalOtherLba = 0;
    uint32_t journalTargetLba = 0;
    uint64_t journalGeneration = 0;
    uint64_t olderGeneration = 0;
    int linkedFromClosed = 0;
    int unlinkedLegacyCandidate = 0;
    int journalSlotBad = 0;
    int found = 0;

    *integrityType = 0xff;
    if (prevailingLba) *prevailingLba = 0;
    if (checksumRepair) *checksumRepair = 0;
    RecoveryAnchorCandidates(reader->SectorCount, anchors);
    for (;;) {
        uint32_t i, count;

        if (!extentBytes || (extentBytes & (UDFS_RECOVERY_SECTOR_SIZE - 1)))
            return UdfsRecoveryProbeCorrupt;
        if (extentBytes > 256 * UDFS_RECOVERY_SECTOR_SIZE)
            return UdfsRecoveryProbeUnsupported;
        count = extentBytes / UDFS_RECOVERY_SECTOR_SIZE;
        if (extentLba >= reader->SectorCount ||
            count > reader->SectorCount - extentLba)
            return UdfsRecoveryProbeCorrupt;
        /* Recovery can rewrite the prevailing LVID. Do not treat a valid
         * descriptor tag inside the physical partition as proof that its
         * sector is reserved for the integrity sequence: it may be owned by
         * a file, metadata extent or bitmap. Refuse this layout before any
         * repair instead of writing through that other owner's sector. */
        if ((uint64_t)extentLba <
                (uint64_t)result->PartitionStart + result->PartitionBlocks &&
            (uint64_t)result->PartitionStart <
                (uint64_t)extentLba + count) {
            result->FailureLba = extentLba > result->PartitionStart ?
                extentLba : result->PartitionStart;
            return UdfsRecoveryProbeCorrupt;
        }
        /* The volume descriptor sequences are allocated extents, including
         * their sectors after the terminating descriptor. A valid LVID tag
         * there or at an anchor candidate is not proof of LVID ownership.
         * An accepted checkpoint may be rewritten during repair. */
        {
            uint32_t starts[2], bytes[2], j;
            starts[0] = result->MainVdsStartLba;
            starts[1] = result->ReserveVdsStartLba;
            bytes[0] = result->MainVdsBytes;
            bytes[1] = result->ReserveVdsBytes;
            for (j = 0; j < 2; ++j) {
                uint32_t blocks;
                if (!bytes[j]) {
                    if (j == 0) return UdfsRecoveryProbeCorrupt;
                    continue;
                }
                if (bytes[j] & (UDFS_RECOVERY_SECTOR_SIZE - 1))
                    return UdfsRecoveryProbeCorrupt;
                blocks = bytes[j] / UDFS_RECOVERY_SECTOR_SIZE;
                if (starts[j] >= reader->SectorCount ||
                    blocks > reader->SectorCount - starts[j])
                    return UdfsRecoveryProbeCorrupt;
                if ((uint64_t)extentLba < (uint64_t)starts[j] + blocks &&
                    (uint64_t)starts[j] < (uint64_t)extentLba + count) {
                    result->FailureLba = extentLba > starts[j] ?
                        extentLba : starts[j];
                    return UdfsRecoveryProbeCorrupt;
                }
            }
            for (j = 0; j < 8; ++j) {
                if (anchors[j] < reader->SectorCount &&
                    anchors[j] >= extentLba &&
                    anchors[j] - extentLba < count) {
                    result->FailureLba = anchors[j];
                    return UdfsRecoveryProbeCorrupt;
                }
            }
        }
        for (i = 0; i < count; ++i) {
            uint32_t lba = extentLba + i;
            uint32_t nextBytes, nextLba;
            uint16_t kind;
            uint8_t repaired = 0;
            UDFS_RECOVERY_PROBE_STATUS status;

            result->FailureLba = lba;
            if (++visited > 1024)
                return UdfsRecoveryProbeUnsupported;
            status = ReadSector(reader, result, lba, sector);
            if (status != UdfsRecoveryProbeValidLayout) return status;
            kind = Read16(sector);
            if (kind == 8) {
                status = ValidateTag(sector, 8, lba);
                if (status != UdfsRecoveryProbeValidLayout) return status;
                if (!found) return UdfsRecoveryProbeCorrupt;
                result->FailureLba = 0;
                *integrityType = lastType;
                return UdfsRecoveryProbeValidLayout;
            }
            if (kind != 9) return UdfsRecoveryProbeCorrupt;
            status = NormalizeOpenLvidTag(sector, lba,
                         linkedFromClosed && count == 1 && i == 0 &&
                         Read16(sector + 2) == previousVersion &&
                         Read16(sector + 6) == previousSerial &&
                         Read16(sector + 10) == previousCrcLength ? 2 : 0,
                         linkedFromClosed ? previousClosed : NULL,
                         &repaired);
            if (status == UdfsRecoveryProbeValidLayout &&
                ((linkedFromClosed && count == 1 && i == 0) ||
                 (unlinkedLegacyCandidate && lba == journalTargetLba)) &&
                Read32(sector + 28) == 0) {
                uint64_t targetGeneration = 0;
                int targetJournaled =
                    Read32(sector + 76) == UDFS_JOURNAL_IMP_LENGTH &&
                    Read16(sector + 10) == UDFS_JOURNAL_CRC_LENGTH &&
                    RecoveryDifference(sector + 142,
                                       UdfsJournalMagic, 8) == 0;
                if (targetJournaled)
                    targetGeneration = Read64(sector + 150);
                if ((journalSlotBad && journalOpenLba &&
                     targetGeneration != journalGeneration) ||
                    (targetJournaled &&
                     (!targetGeneration ||
                      Read32(sector + 158) != lba ||
                      Read32(sector + 162) > 1 ||
                      !journalOpenLba ||
                      targetGeneration > journalGeneration)))
                    return UdfsRecoveryProbeCorrupt;
                if (journalOpenLba && journalTargetLba == lba) {
                    if (targetGeneration < journalGeneration) {
                        uint64_t gap =
                            journalGeneration - targetGeneration;
                        if (gap != 1 &&
                            !(gap == 2 &&
                              Read32(journalOpen + 162) == 1))
                            return UdfsRecoveryProbeCorrupt;
                        status = NormalizeLvidFromJournal(
                            sector, lba, journalOpen,
                            journalOpenLba,
                            olderGeneration &&
                            olderGeneration + 1 == journalGeneration ?
                                olderJournal : NULL, &repaired);
                    } else if (targetGeneration == journalGeneration) {
                        memcpy(expected, journalOpen,
                               UDFS_RECOVERY_SECTOR_SIZE);
                        RetagLvid(expected, lba);
                        if (RecoveryDifference(sector, expected,
                                               UDFS_RECOVERY_SECTOR_SIZE))
                            return UdfsRecoveryProbeCorrupt;
                        /* Keep the validated sidecar identity even when the
                         * target OPEN tag is intact. Repair may need a new
                         * generation to publish corrected free counters. */
                        result->PrecedingClosedLvidLba = previousClosedLba;
                        result->JournalOpenLvidLba = journalOpenLba;
                        result->JournalOtherLvidLba = journalOtherLba;
                        result->JournalGeneration = journalGeneration;
                        result->JournalOtherDamaged =
                            (uint8_t)journalSlotBad;
                    }
                }
            }
            if (status == UdfsRecoveryProbeCorrupt &&
                journalOpenLba && journalTargetLba == lba &&
                ((linkedFromClosed && count == 1 && i == 0) ||
                 unlinkedLegacyCandidate)) {
                status = NormalizeLvidFromJournal(
                    sector, lba, journalOpen, journalOpenLba,
                    olderGeneration &&
                    olderGeneration + 1 == journalGeneration ?
                        olderJournal : NULL, &repaired);
            }
            if (status != UdfsRecoveryProbeValidLayout) return status;
            if (repaired && checksumRepair) *checksumRepair = repaired;
            if (repaired == 2)
                result->PrecedingClosedLvidLba = previousClosedLba;
            if (repaired == 3) {
                result->PrecedingClosedLvidLba = previousClosedLba;
                result->JournalOpenLvidLba = journalOpenLba;
                result->JournalOtherLvidLba = journalOtherLba;
                result->JournalGeneration = journalGeneration;
            }
            /* A CLOSED decision requires CRC coverage of the integrity type,
             * free/size tables and counters, not just a valid tag checksum. */
            if (Read16(sector + 10) < 136 - 16)
                return UdfsRecoveryProbeCorrupt;
            if (Read32(sector + 72) != 2 || Read32(sector + 28) > 1)
                return UdfsRecoveryProbeUnsupported;
            lastType = (uint8_t)Read32(sector + 28);
            if (prevailing) memcpy(prevailing, sector,
                                   UDFS_RECOVERY_SECTOR_SIZE);
            if (prevailingLba) *prevailingLba = lba;
            found = 1;
            nextBytes = Read32(sector + 32);
            if (!nextBytes) {
                /* Legacy fixed disks may contain two contiguous LVIDs and
                 * no linked CLOSED predecessor. A journaled successor is
                 * considered only when the second sector is immediately
                 * followed by a valid terminator and both sidecar sectors
                 * lie at the end of this same integrity extent. Without a
                 * valid sidecar the old fail-closed behavior is unchanged. */
                if (i == 0 && count >= 5 && i + 2 < count - 2) {
                    uint32_t slot, target = lba + 1;
                    int conflict = 0;
                    status = ReadSector(reader, result, lba + 2,
                                        terminator);
                    if (status != UdfsRecoveryProbeValidLayout)
                        return status;
                    if (ValidateTag(terminator, 8, lba + 2) ==
                            UdfsRecoveryProbeValidLayout) {
                        unlinkedLegacyCandidate = 1;
                        previousClosedLba = lba;
                        journalTargetLba = target;
                        memcpy(previousClosed, sector,
                               UDFS_RECOVERY_SECTOR_SIZE);
                        for (slot = 0; slot < 2; ++slot) {
                            uint32_t slotLba =
                                extentLba + count - 2 + slot;
                            uint64_t generation;
                            uint32_t k;
                            status = ReadSector(reader, result, slotLba,
                                                candidate);
                            if (status != UdfsRecoveryProbeValidLayout)
                                return status;
                            if (!JournalOpenValid(candidate, slotLba,
                                                  target, sector)) {
                                for (k = 0; k < UDFS_RECOVERY_SECTOR_SIZE; ++k)
                                    if (candidate[k]) {
                                        journalSlotBad = 1;
                                        break;
                                    }
                                continue;
                            }
                            generation = Read64(candidate + 150);
                            if (generation == journalGeneration) {
                                conflict = 1;
                            } else if (generation > journalGeneration) {
                                if (journalGeneration) {
                                    olderGeneration = journalGeneration;
                                    memcpy(olderJournal, journalOpen,
                                           UDFS_RECOVERY_SECTOR_SIZE);
                                }
                                journalOpenLba = slotLba;
                                journalOtherLba = extentLba + count - 1 - slot;
                                journalGeneration = generation;
                                journalTargetLba = target;
                                memcpy(journalOpen, candidate,
                                       UDFS_RECOVERY_SECTOR_SIZE);
                            } else {
                                olderGeneration = generation;
                                memcpy(olderJournal, candidate,
                                       UDFS_RECOVERY_SECTOR_SIZE);
                            }
                        }
                        if (conflict) {
                            journalOpenLba = 0;
                            journalGeneration = 0;
                        }
                    }
                }
                continue;
            }
            nextLba = Read32(sector + 36);
            if (nextLba == lba) return UdfsRecoveryProbeCorrupt;
            journalOpenLba = 0;
            journalOtherLba = 0;
            journalGeneration = 0;
            olderGeneration = 0;
            journalTargetLba = 0;
            journalSlotBad = 0;
            unlinkedLegacyCandidate = 0;
            if (lastType == 1 && count >= 5 &&
                extentLba + count - 2 > lba + 1 &&
                nextLba != extentLba + count - 2 &&
                nextLba != extentLba + count - 1) {
                uint32_t slot;
                int conflict = 0;
                for (slot = 0; slot < 2; ++slot) {
                    uint32_t slotLba = extentLba + count - 2 + slot;
                    uint64_t generation;
                    uint32_t k;
                    if (ReadSector(reader, result, slotLba, candidate) !=
                            UdfsRecoveryProbeValidLayout) {
                        journalSlotBad = 1;
                        continue;
                    }
                    if (!JournalOpenValid(candidate, slotLba, nextLba,
                                          sector)) {
                        for (k = 0; k < UDFS_RECOVERY_SECTOR_SIZE; ++k)
                            if (candidate[k]) {
                                journalSlotBad = 1;
                                break;
                            }
                        continue;
                    }
                    generation = Read64(candidate + 150);
                    if (generation == journalGeneration) {
                        conflict = 1;
                    } else if (generation > journalGeneration) {
                        if (journalGeneration) {
                            olderGeneration = journalGeneration;
                            memcpy(olderJournal, journalOpen,
                                   UDFS_RECOVERY_SECTOR_SIZE);
                        }
                        journalOpenLba = slotLba;
                        journalOtherLba = extentLba + count - 1 - slot;
                        journalGeneration = generation;
                        journalTargetLba = nextLba;
                        memcpy(journalOpen, candidate,
                               UDFS_RECOVERY_SECTOR_SIZE);
                    } else {
                        olderGeneration = generation;
                        memcpy(olderJournal, candidate,
                               UDFS_RECOVERY_SECTOR_SIZE);
                    }
                }
                if (conflict) {
                    journalOpenLba = 0;
                    journalOtherLba = 0;
                    journalGeneration = 0;
                    olderGeneration = 0;
                    journalTargetLba = 0;
                }
            }
            linkedFromClosed = lastType == 1;
            previousVersion = Read16(sector + 2);
            previousSerial = Read16(sector + 6);
            previousCrcLength = Read16(sector + 10);
            if (linkedFromClosed) {
                memcpy(previousClosed, sector, UDFS_RECOVERY_SECTOR_SIZE);
                previousClosedLba = lba;
            }
            extentBytes = nextBytes;
            extentLba = nextLba;
            goto next_extent;
        }
        if (!found) return UdfsRecoveryProbeCorrupt;
        result->FailureLba = 0;
        *integrityType = lastType;
        return UdfsRecoveryProbeValidLayout;
next_extent:
        ;
    }
}

static UDFS_RECOVERY_PROBE_STATUS
ReadPrevailingIntegrity(const UDFS_RECOVERY_READER *reader,
                        UDFS_RECOVERY_PROBE *result, uint32_t extentBytes,
                        uint32_t extentLba, uint8_t *sector,
                        uint8_t *prevailing, uint32_t *prevailingLba,
                        uint8_t *integrityType,
                        uint8_t *checksumRepair)
{
    RECOVERY_INTEGRITY_WORK *work;
    UDFS_RECOVERY_PROBE_STATUS status;

    if (!reader || !reader->Allocate || !reader->Free)
        return UdfsRecoveryProbeInvalidInput;
    work = (RECOVERY_INTEGRITY_WORK *)reader->Allocate(reader->Context,
                                                       sizeof(*work));
    if (!work) return UdfsRecoveryProbeNoMemory;
    memset(work, 0, sizeof(*work));
    status = ReadPrevailingIntegrityWorker(reader, result, extentBytes,
                                           extentLba, sector, prevailing,
                                           prevailingLba, integrityType,
                                           checksumRepair, work);
    reader->Free(reader->Context, work);
    return status;
}

typedef struct _RECOVERY_PEEK_WORK {
    uint8_t Sector[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Pd[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Lvd[UDFS_RECOVERY_SECTOR_SIZE];
    UDFS_RECOVERY_PROBE Scratch;
} RECOVERY_PEEK_WORK;

UDFS_RECOVERY_PROBE_STATUS
UdfsRecoveryPeekIntegrity(const UDFS_RECOVERY_READER *reader,
                          uint8_t *integrityType)
{
    RECOVERY_PEEK_WORK *work;
    UDFS_RECOVERY_PROBE *scratch;
    uint8_t *sector, *pd, *lvd;
    UDFS_RECOVERY_PROBE_STATUS status;
    uint32_t extentBytes, extentLba;

    if (!reader || !reader->ReadSector || !reader->Allocate ||
        !reader->Free || !integrityType || reader->SectorCount <= 256)
        return UdfsRecoveryProbeInvalidInput;
    *integrityType = 0xff;
    work = (RECOVERY_PEEK_WORK *)reader->Allocate(reader->Context,
                                                  sizeof(*work));
    if (!work) return UdfsRecoveryProbeNoMemory;
    memset(work, 0, sizeof(*work));
    sector = work->Sector;
    pd = work->Pd;
    lvd = work->Lvd;
    scratch = &work->Scratch;
    status = SelectRecoveryDescriptors(reader, scratch, sector, pd, lvd);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    if (Read32(lvd + 212) != UDFS_RECOVERY_SECTOR_SIZE ||
        Read32(lvd + 268) != 2 || Read32(lvd + 264) != 70 ||
        lvd[440] != 1 || lvd[441] != 6 ||
        lvd[446] != 2 || lvd[447] != 64 ||
        RecoveryDifference(lvd + 451,
                           "*UDF Metadata Partition", 23) != 0 ||
        lvd[504] != 1) {
        status = UdfsRecoveryProbeUnsupported;
        goto done;
    }
    scratch->PartitionStart = Read32(pd + 188);
    scratch->PartitionBlocks = Read32(pd + 192);
    if (!scratch->PartitionBlocks ||
        scratch->PartitionStart >= reader->SectorCount ||
        scratch->PartitionBlocks >
            reader->SectorCount - scratch->PartitionStart) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }

    extentBytes = Read32(lvd + 432) & 0x3fffffff;
    extentLba = Read32(lvd + 436);
    status = ReadPrevailingIntegrity(reader, scratch, extentBytes, extentLba,
                                     sector, NULL, NULL, integrityType, NULL);
done:
    reader->Free(reader->Context, work);
    return status;
}

static UDFS_RECOVERY_PROBE_STATUS
ProbeLayout(const UDFS_RECOVERY_READER *reader,
             UDFS_RECOVERY_PROBE *result, UDFS_RECOVERY_REPAIR_PLAN *plan)
{
    RECOVERY_LAYOUT_WORK *work;
    uint8_t *sector, *pd, *lvd;
    uint8_t *mainSector, *mirrorSector, *prevailingLvid;
    RECOVERY_SECTOR_CACHE *physicalCache, *metadataCache;
    RECOVERY_EXTENT_LIST *mainFile, *mirrorFile, *bitmapFile;
    UDFS_RECOVERY_PROBE_STATUS status;
    uint32_t i, partitionNumber;
    uint32_t mapLength, bitmapBytes, bitmapExtentBytes;
    const uint8_t *metadataMap;

    if (!result) return UdfsRecoveryProbeInvalidInput;
    memset(result, 0, sizeof(*result));
    result->FirstReferencedFreePhysicalLbn = UINT32_MAX;
    result->FirstReferencedFreeMetadataLbn = UINT32_MAX;
    result->FirstUnownedPhysicalLbn = UINT32_MAX;
    result->FirstUnownedMetadataLbn = UINT32_MAX;
    result->Status = UdfsRecoveryProbeInvalidInput;
    if (!reader || !reader->ReadSector || !reader->Allocate ||
        !reader->Free || reader->SectorCount <= 256)
        return result->Status;
    work = (RECOVERY_LAYOUT_WORK *)reader->Allocate(reader->Context,
                                                    sizeof(*work));
    if (!work) {
        result->Status = UdfsRecoveryProbeNoMemory;
        return result->Status;
    }
    memset(work, 0, sizeof(*work));
    sector = work->Sector;
    pd = work->Pd;
    lvd = work->Lvd;
    mainSector = work->MainSector;
    mirrorSector = work->MirrorSector;
    prevailingLvid = work->PrevailingLvid;
    physicalCache = &work->PhysicalCache;
    metadataCache = &work->MetadataCache;
    for (i = 0; i < 3; ++i) {
        work->Lists[i].Items = work->ListItems[i];
        work->Lists[i].Capacity = UDFS_RECOVERY_MAX_EXTENTS;
    }
    mainFile = &work->Lists[0];
    mirrorFile = &work->Lists[1];
    bitmapFile = &work->Lists[2];

    status = SelectRecoveryDescriptors(reader, result, sector, pd, lvd);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    if (Read32(lvd + 212) != UDFS_RECOVERY_SECTOR_SIZE) {
        status = UdfsRecoveryProbeUnsupported;
        goto done;
    }
    partitionNumber = Read16(pd + 22);
    result->PartitionStart = Read32(pd + 188);
    result->PartitionBlocks = Read32(pd + 192);
    if (!result->PartitionBlocks ||
        result->PartitionStart >= reader->SectorCount ||
        result->PartitionBlocks > reader->SectorCount - result->PartitionStart) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }
    mapLength = Read32(lvd + 264);
    if (Read32(lvd + 268) != 2 || mapLength != 70 ||
        lvd[440] != 1 || lvd[441] != 6 ||
        Read16(lvd + 444) != partitionNumber ||
        lvd[446] != 2 || lvd[447] != 64) {
        status = UdfsRecoveryProbeUnsupported;
        goto done;
    }
    metadataMap = lvd + 446;
    if (RecoveryDifference(metadataMap + 5, "*UDF Metadata Partition", 23) != 0 ||
        Read16(metadataMap + 38) != partitionNumber ||
        (metadataMap[58] & ~1U)) {
        status = UdfsRecoveryProbeUnsupported;
        goto done;
    }
    result->MetadataDuplicated = metadataMap[58] & 1;
    result->MetadataMainFeLbn = Read32(metadataMap + 40);
    result->MetadataMirrorFeLbn = Read32(metadataMap + 44);
    result->MetadataBitmapFeLbn = Read32(metadataMap + 48);

    status = ReadFileExtents(reader, result, result->MetadataMainFeLbn,
                             250, mainFile);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = ReadFileExtents(reader, result, result->MetadataMirrorFeLbn,
                             251, mirrorFile);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    if (mainFile->FileBytes != mirrorFile->FileBytes ||
        mainFile->FileBytes != (uint64_t)mainFile->TotalBlocks *
                              UDFS_RECOVERY_SECTOR_SIZE ||
        mirrorFile->FileBytes != (uint64_t)mirrorFile->TotalBlocks *
                                UDFS_RECOVERY_SECTOR_SIZE) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }
    result->MetadataBlocks = mainFile->TotalBlocks;
    result->MainExtents = mainFile->Count;
    result->MirrorExtents = mirrorFile->Count;
    if (result->MetadataDuplicated) {
        uint32_t m, n;
        for (m = 0; m < mainFile->Count; ++m)
            for (n = 0; n < mirrorFile->Count; ++n)
                if (RangesOverlap(&mainFile->Items[m],
                                  &mirrorFile->Items[n])) {
                    status = UdfsRecoveryProbeCorrupt;
                    goto done;
                }
    } else if (mainFile->Count != mirrorFile->Count ||
               RecoveryDifference(mainFile->Items, mirrorFile->Items,
                      mainFile->Count * sizeof(RECOVERY_EXTENT)) != 0) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }

    status = ReadFileExtents(reader, result, result->MetadataBitmapFeLbn,
                             252, bitmapFile);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    {
        /* The first bitmap descriptor is no longer needed when mirror
         * comparisons start, so reuse the existing heap sector. */
        uint8_t *descriptor = mainSector;
        uint32_t bitmapLba, bitmapOffset, tagLbn;
        status = MapExtentByte(result, bitmapFile, 0,
                               &bitmapLba, &bitmapOffset);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        status = ReadSector(reader, result, bitmapLba, sector);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        if (bitmapFile->Inline) {
            if (bitmapFile->FileBytes >
                UDFS_RECOVERY_SECTOR_SIZE - bitmapOffset) {
                status = UdfsRecoveryProbeCorrupt;
                goto done;
            }
            memset(descriptor, 0, UDFS_RECOVERY_SECTOR_SIZE);
            memcpy(descriptor, sector + bitmapOffset,
                   (size_t)bitmapFile->FileBytes);
            if (Read16(descriptor + 10) > bitmapFile->FileBytes - 16) {
                status = UdfsRecoveryProbeCorrupt;
                goto done;
            }
            tagLbn = bitmapFile->InlineFeLbn;
        } else {
            if (bitmapOffset || !bitmapFile->Count) {
                status = UdfsRecoveryProbeCorrupt;
                goto done;
            }
            memcpy(descriptor, sector, UDFS_RECOVERY_SECTOR_SIZE);
            tagLbn = bitmapFile->Items[0].Lbn;
        }
        status = ValidateTag(descriptor, 0x108, tagLbn);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        result->MetadataBitmapBits = Read32(descriptor + 16);
        bitmapBytes = Read32(descriptor + 20);
        if (result->MetadataBitmapBits < result->MetadataBlocks ||
            bitmapBytes < (result->MetadataBitmapBits + 7U) / 8U ||
            bitmapFile->FileBytes < 24ULL + bitmapBytes) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
    }

    bitmapExtentBytes = Read32(pd + 64) & 0x3fffffff;
    result->PhysicalBitmapLbn = Read32(pd + 68);
    if (bitmapExtentBytes < 24ULL + (result->PartitionBlocks + 7ULL) / 8ULL ||
        result->PhysicalBitmapLbn >= result->PartitionBlocks ||
        (bitmapExtentBytes + 511ULL) / 512ULL >
            result->PartitionBlocks - result->PhysicalBitmapLbn) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }
    status = ReadSector(reader, result,
                        result->PartitionStart + result->PhysicalBitmapLbn,
                        sector);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    status = ValidateTag(sector, 0x108, result->PhysicalBitmapLbn);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    if (Read32(sector + 16) != result->PartitionBlocks ||
        Read32(sector + 20) < (result->PartitionBlocks + 7ULL) / 8ULL) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }

    for (i = 0; i < (result->PartitionBlocks + 7ULL) / 8; ++i) {
        uint32_t byteOffset = 24 + i;
        uint32_t remaining = result->PartitionBlocks - i * 8;
        uint8_t value;
        status = ReadCachedByte(reader, result, physicalCache,
                    result->PartitionStart + result->PhysicalBitmapLbn +
                    byteOffset / UDFS_RECOVERY_SECTOR_SIZE,
                    byteOffset % UDFS_RECOVERY_SECTOR_SIZE, &value);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        if (remaining < 8) value &= (uint8_t)((1U << remaining) - 1);
        result->PhysicalFreeBlocks += CountByteBits(value);
    }

    for (i = 0; i < result->MetadataBlocks; ++i) {
        uint32_t bitmapLba, bitmapOffset;
        uint8_t value;
        status = MapExtentByte(result, bitmapFile, 24ULL + (i >> 3),
                               &bitmapLba, &bitmapOffset);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        status = ReadCachedByte(reader, result, metadataCache,
                                bitmapLba, bitmapOffset, &value);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        if ((value >> (i & 7)) & 1) {
            result->MetadataFreeBlocks++;
        } else if (result->MetadataDuplicated) {
            uint32_t mainLba, mirrorLba;
            status = MapMetadataBlock(result, mainFile, i, &mainLba);
            if (status != UdfsRecoveryProbeValidLayout) goto done;
            status = MapMetadataBlock(result, mirrorFile, i, &mirrorLba);
            if (status != UdfsRecoveryProbeValidLayout) goto done;
            status = ReadSector(reader, result, mainLba, mainSector);
            if (status != UdfsRecoveryProbeValidLayout) goto done;
            status = ReadSector(reader, result, mirrorLba, mirrorSector);
            if (status != UdfsRecoveryProbeValidLayout) goto done;
            if (RecoveryDifference(mainSector, mirrorSector,
                       UDFS_RECOVERY_SECTOR_SIZE) != 0) {
                int liveOnMirror;
                uint32_t childLbn, recordOffset;
                if (result->MirrorDirectorySplitCount ||
                    result->FidSplitCount) {
                    result->FailureLba = mainLba;
                    status = UdfsRecoveryProbeCorrupt;
                    goto done;
                }
                if (DirectoryFePairDiffersOnlyInLinkCount(
                        mainSector, mirrorSector, i)) {
                    result->MirrorDirectorySplitCount = 1;
                    result->MirrorDirectoryLbn = i;
                    result->MirrorDirectoryMainLinks =
                        Read16(mainSector + 48);
                    result->MirrorDirectoryMirrorLinks =
                        Read16(mirrorSector + 48);
                } else if (DirectoryFidPairLiveDeleted(
                               mainSector, mirrorSector, i,
                               &liveOnMirror, &childLbn,
                               &recordOffset)) {
                    result->FidSplitCount = 1;
                    result->FidSplitLbn = i;
                    result->FidSplitMainLba = mainLba;
                    result->FidSplitMirrorLba = mirrorLba;
                    result->FidSplitChildLbn = childLbn;
                    result->FidSplitRecordOffset = recordOffset;
                    result->FidSplitLiveOnMirror =
                        (uint8_t)liveOnMirror;
                } else {
                    result->FailureLba = mainLba;
                    status = UdfsRecoveryProbeCorrupt;
                    goto done;
                }
            }
            result->AllocatedMirrorBlocksCompared++;
        }
    }
    result->BitmapContentsScanned = 1;

    status = ReadPrevailingIntegrity(reader, result,
                Read32(lvd + 432) & 0x3fffffff, Read32(lvd + 436),
                sector, prevailingLvid, &result->PrevailingLvidLba,
                &result->LvidIntegrityType,
                &result->LvidTagRepairMode);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    if (Read16(prevailingLvid + 10) < 136 - 16) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }
    if (Read32(prevailingLvid + 72) != 2 ||
        Read32(prevailingLvid + 28) > 1) {
        status = UdfsRecoveryProbeUnsupported;
        goto done;
    }
    result->LvidIntegrityType = (uint8_t)Read32(prevailingLvid + 28);
    result->LvidPhysicalFreeBlocks = Read32(prevailingLvid + 80);
    result->LvidMetadataFreeBlocks = Read32(prevailingLvid + 84);
    result->LvidPhysicalSizeBlocks = Read32(prevailingLvid + 88);
    result->LvidMetadataSizeBlocks = Read32(prevailingLvid + 92);
    result->LvidNextUniqueId = Read64(prevailingLvid + 40);
    result->LvidFileCount = Read32(prevailingLvid + 128);
    result->LvidDirectoryCount = Read32(prevailingLvid + 132);
    result->LvidCountersMatch =
        result->LvidPhysicalFreeBlocks == result->PhysicalFreeBlocks &&
        result->LvidMetadataFreeBlocks == result->MetadataFreeBlocks &&
        result->LvidPhysicalSizeBlocks == result->PartitionBlocks &&
        result->LvidMetadataSizeBlocks == result->MetadataBlocks;
    status = GraphRun(reader, result, mainFile, mirrorFile,
                      bitmapFile, lvd, bitmapExtentBytes, plan, sector);
    if (result->OwnershipGraphScanned)
        result->LvidCountersMatch = result->LvidCountersMatch &&
            result->LvidFileCount == result->FilesVisited &&
            result->LvidDirectoryCount == result->DirectoriesVisited &&
            result->LvidNextUniqueId > result->MaxUniqueId;
    if (status == UdfsRecoveryProbeValidLayout &&
        !result->LvidCountersMatch)
        status = UdfsRecoveryProbeLvidMismatch;
    /* Probing is read-only. Any checkpoint still requires a repair plan,
     * lower-device barriers and verification by UdfsRecoveryApplyRepair. */
done:
    result->Status = status;
    reader->Free(reader->Context, work);
    return status;
}

UDFS_RECOVERY_PROBE_STATUS
UdfsRecoveryProbeLayout(const UDFS_RECOVERY_READER *reader,
                        UDFS_RECOVERY_PROBE *result)
{
    return ProbeLayout(reader, result, NULL);
}

/* Roll back only physical allocations absent from every durable owner. An
 * OPEN checkpoint can predate both reachable new allocations and orphans;
 * the former must stay allocated and the latter alone may be freed. The
 * complete owner graph and free-metadata FE scan are mandatory. A changed
 * LVID free count must be journaled before publishing CLOSED. */
static int
GraphCanRollbackPhysicalOrphans(const UDFS_RECOVERY_PROBE *probe)
{
    return probe->Status == UdfsRecoveryProbeGraphMismatch &&
        probe->LvidIntegrityType == 0 && probe->MetadataDuplicated &&
        probe->BitmapContentsScanned && probe->OwnershipGraphScanned &&
        probe->FreeMetadataFeScanComplete &&
        probe->UnownedPhysicalBlocks && !probe->UnownedMetadataBlocks &&
        !probe->ReferencedFreePhysicalBlocks &&
        !probe->ReferencedFreeMetadataBlocks && !probe->OverlapBlocks &&
        !probe->FidSplitCount && !probe->MirrorDirectorySplitCount &&
        !probe->DirectoryLinkFixCount &&
        probe->LvidPhysicalSizeBlocks == probe->PartitionBlocks &&
        probe->LvidMetadataSizeBlocks == probe->MetadataBlocks &&
        probe->LvidMetadataFreeBlocks == probe->MetadataFreeBlocks &&
        probe->LvidFileCount == probe->FilesVisited &&
        probe->LvidDirectoryCount == probe->DirectoriesVisited &&
        probe->LvidNextUniqueId > probe->MaxUniqueId &&
        probe->LvidPhysicalFreeBlocks > probe->PhysicalFreeBlocks &&
        probe->LvidPhysicalFreeBlocks <= probe->PartitionBlocks &&
        probe->LvidPhysicalFreeBlocks - probe->PhysicalFreeBlocks >=
            probe->UnownedPhysicalBlocks &&
        probe->OwnedPhysicalBlocks >= probe->PartitionBlocks -
            probe->LvidPhysicalFreeBlocks &&
        (uint64_t)probe->OwnedPhysicalBlocks +
            probe->UnownedPhysicalBlocks + probe->PhysicalFreeBlocks ==
            probe->PartitionBlocks;
}

/* A fully walked, duplicate-free namespace with an exact physical owner
 * graph may account for metadata still marked free. A separately validated
 * physical-only rollback is the sole exception to exact ownership. */
static int
GraphCanRecover(const UDFS_RECOVERY_PROBE *probe)
{
    return probe->LvidIntegrityType == 0 && probe->MetadataDuplicated &&
        probe->BitmapContentsScanned && probe->OwnershipGraphScanned &&
        (!probe->FidSplitCount ||
         (probe->FidSplitCount == 1 &&
          !probe->MirrorDirectorySplitCount &&
          probe->FidSplitSeen &&
          probe->DirectoryLinkFixCount == 1 &&
          probe->FidSplitParentLbn ==
              probe->DirectoryLinkFixParentLbn &&
          probe->FidSplitChildLbn ==
              probe->DirectoryLinkFixChildLbn)) &&
        (!probe->MirrorDirectorySplitCount ||
         (probe->MirrorDirectorySplitCount == 1 &&
          probe->DirectoryLinkFixCount == 1 &&
          probe->DirectoryLinkFixParentLbn ==
              probe->MirrorDirectoryLbn)) &&
        (!probe->DirectoryLinkFixCount ||
         (probe->DirectoryLinkFixCount == 1 &&
          (probe->MirrorDirectorySplitCount ||
           (probe->DirectoryLinkFixChildVerified &&
            probe->ReferencedFreePhysicalBlocks == 0 &&
            probe->ReferencedFreeMetadataBlocks == 1 &&
            probe->FirstReferencedFreeMetadataLbn ==
                probe->DirectoryLinkFixChildLbn)))) &&
        (!probe->UnownedPhysicalBlocks ||
         GraphCanRollbackPhysicalOrphans(probe)) &&
        !probe->UnownedMetadataBlocks && !probe->OverlapBlocks &&
        probe->ReferencedFreeMirrorBlocksCompared ==
            probe->ReferencedFreeMetadataBlocks &&
        probe->LvidPhysicalFreeBlocks <= probe->PartitionBlocks &&
        probe->LvidPhysicalSizeBlocks == probe->PartitionBlocks &&
        probe->PhysicalFreeBlocks >= probe->ReferencedFreePhysicalBlocks &&
        probe->MetadataFreeBlocks >= probe->ReferencedFreeMetadataBlocks &&
        probe->MaxUniqueId != UINT64_MAX &&
        (probe->Status == UdfsRecoveryProbeValidLayout ||
         probe->Status == UdfsRecoveryProbeGraphMismatch ||
         probe->Status == UdfsRecoveryProbeLvidMismatch);
}

typedef struct _LEGACY_BOOTSTRAP_SCRATCH {
    UDFS_RECOVERY_PROBE Check;
    uint8_t Sector[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Pd[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Lvd[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Terminator[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t Other[UDFS_RECOVERY_SECTOR_SIZE];
} LEGACY_BOOTSTRAP_SCRATCH;

/* An old two-LVID disk can have an OPEN first descriptor with no linked
 * predecessor. Its reserved integrity-extent tail is available for the
 * same two-slot journal used by ordinary writes, but recovery must prepare
 * that journal before it may publish a CLOSED final descriptor. This helper
 * only recognizes that exact on-disk layout; other legacy layouts retain
 * their existing recovery policy. The validated first LVID and both empty
 * slots become part of the stale-plan check in ApplyRepair. */
static UDFS_RECOVERY_PROBE_STATUS
PrepareLegacyJournalBootstrapInner(const UDFS_RECOVERY_READER *reader,
                                   const UDFS_RECOVERY_PROBE *probe,
                                   const uint8_t recordingTime[12],
                                   UDFS_RECOVERY_REPAIR_PLAN *plan,
                                   LEGACY_BOOTSTRAP_SCRATCH *scratch)
{
    UDFS_RECOVERY_PROBE *check = &scratch->Check;
    UDFS_RECOVERY_PROBE_STATUS status;
    uint8_t *sector = scratch->Sector;
    uint8_t *pd = scratch->Pd;
    uint8_t *lvd = scratch->Lvd;
    uint8_t *terminator = scratch->Terminator;
    uint8_t *other = scratch->Other;
    UDFS_RECOVERY_SECTOR_EDIT *edit = &plan->LvidEdit;
    uint32_t extentLba, extentBytes, count, slot, i;
    int interrupted = 0;

    memset(check, 0, sizeof(*check));
    status = SelectRecoveryDescriptors(reader, check, sector, pd, lvd);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    if (check->AnchorLba != probe->AnchorLba ||
        check->VdsStartLba != probe->VdsStartLba)
        return UdfsRecoveryProbeCorrupt;
    extentBytes = Read32(lvd + 432) & 0x3fffffff;
    extentLba = Read32(lvd + 436);
    if ((extentBytes & (UDFS_RECOVERY_SECTOR_SIZE - 1)) ||
        extentBytes < 5 * UDFS_RECOVERY_SECTOR_SIZE ||
        extentBytes > 256 * UDFS_RECOVERY_SECTOR_SIZE ||
        extentLba >= reader->SectorCount)
        return UdfsRecoveryProbeValidLayout;
    count = extentBytes / UDFS_RECOVERY_SECTOR_SIZE;
    if (count > reader->SectorCount - extentLba ||
        extentLba + 1 != edit->Lba ||
        extentLba + count > probe->PartitionStart ||
        extentLba + count - 2 <= edit->Lba + 1)
        return UdfsRecoveryProbeValidLayout;

    status = ReadSector(reader, check, extentLba + 2, terminator);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    if (ValidateTag(terminator, 8, extentLba + 2) !=
            UdfsRecoveryProbeValidLayout)
        return UdfsRecoveryProbeValidLayout;
    status = ReadSector(reader, check, extentLba,
                        plan->PrecedingClosedLvid);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    if (ValidateTag(plan->PrecedingClosedLvid, 9, extentLba) !=
            UdfsRecoveryProbeValidLayout ||
        Read32(plan->PrecedingClosedLvid + 28) != 0 ||
        Read32(plan->PrecedingClosedLvid + 32) != 0 ||
        Read32(plan->PrecedingClosedLvid + 36) != 0 ||
        Read16(plan->PrecedingClosedLvid + 2) !=
            Read16(edit->Before + 2) ||
        Read16(plan->PrecedingClosedLvid + 6) !=
            Read16(edit->Before + 6))
        return UdfsRecoveryProbeValidLayout;
    plan->PrecedingClosedLvidLba = extentLba;

    /* The alternate tail sector must still be empty. Slot zero may contain
     * our own interrupted bootstrap write; compare it with the newly
     * prepared exact journal below, before allowing it to be overwritten. */
    for (i = 0; i < 2; ++i) {
        slot = extentLba + count - 2 + i;
        status = ReadSector(reader, check, slot,
                            i ? other : plan->JournalNextLvidEdit.Before);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (i)
            for (slot = 0; slot < UDFS_RECOVERY_SECTOR_SIZE; ++slot)
                if (other[slot]) return UdfsRecoveryProbeCorrupt;
    }
    plan->JournalNextLvidEdit.Lba = extentLba + count - 2;

    /* A new close must record the recovery time. If the first sidecar write
     * was interrupted, use its already recorded time on retry so the exact
     * journal bytes remain stable even when the system clock advances. A
     * partial sidecar without a complete valid time fails closed. */
    for (i = 0; i < UDFS_RECOVERY_SECTOR_SIZE; ++i)
        if (plan->JournalNextLvidEdit.Before[i]) {
            interrupted = 1;
            break;
        }
    if (interrupted) {
        if (!ValidLvidRecordingTime(
                plan->JournalNextLvidEdit.Before + 16))
            return UdfsRecoveryProbeCorrupt;
        memcpy(edit->After + 16,
               plan->JournalNextLvidEdit.Before + 16, 12);
    } else {
        if (!ValidLvidRecordingTime(recordingTime))
            return UdfsRecoveryProbeInvalidInput;
        memcpy(edit->After + 16, recordingTime, 12);
    }
    Write16(edit->After + 10, UDFS_JOURNAL_CRC_LENGTH);
    Write32(edit->After + 76, UDFS_JOURNAL_IMP_LENGTH);
    memcpy(edit->After + 142, UdfsJournalMagic, 8);
    Write64(edit->After + 150, 1);
    Write32(edit->After + 158, edit->Lba);
    Write32(edit->After + 162, 1); /* durable recovery prepare */
    plan->LegacyJournalBootstrap = 1;
    return UdfsRecoveryProbeValidLayout;
}

static UDFS_RECOVERY_PROBE_STATUS
PrepareLegacyJournalBootstrap(const UDFS_RECOVERY_READER *reader,
                              const UDFS_RECOVERY_PROBE *probe,
                              const uint8_t recordingTime[12],
                              UDFS_RECOVERY_REPAIR_PLAN *plan)
{
    LEGACY_BOOTSTRAP_SCRATCH *scratch;
    UDFS_RECOVERY_PROBE_STATUS status;
    const UDFS_RECOVERY_SECTOR_EDIT *edit = &plan->LvidEdit;

    if (probe->LvidTagRepairMode != 0 ||
        probe->LvidIntegrityType != 0 ||
        Read32(edit->Before + 28) != 0 ||
        Read16(edit->Before + 10) != UDFS_LEGACY_LVID_CRC_LENGTH ||
        Read32(edit->Before + 76) != 46)
        return UdfsRecoveryProbeValidLayout;

    scratch = (LEGACY_BOOTSTRAP_SCRATCH *)reader->Allocate(
        reader->Context, sizeof(*scratch));
    if (!scratch) return UdfsRecoveryProbeNoMemory;
    status = PrepareLegacyJournalBootstrapInner(reader, probe,
                                                 recordingTime, plan,
                                                 scratch);
    reader->Free(reader->Context, scratch);
    return status;
}

/* A torn write to an initially zero sidecar may leave a correct tag header
 * and a subset of the intended nonzero bytes. It is safe to replace only
 * that exact partial image: no bytes are inferred from it, and the final
 * target is still a CRC-valid legacy OPEN LVID. A conflicting or ambiguous
 * slot remains fail-closed. */
static int
BootstrapSlotReplaceable(const uint8_t *before, const uint8_t *after)
{
    uint32_t i;
    int empty = 1;
    for (i = 0; i < UDFS_RECOVERY_SECTOR_SIZE; ++i)
        if (before[i]) { empty = 0; break; }
    if (empty) return 1;
    if (RecoveryDifference(before, after, 16) != 0) return 0;
    for (i = 16; i < UDFS_RECOVERY_SECTOR_SIZE; ++i)
        if (before[i] && before[i] != after[i]) return 0;
    return 1;
}

UDFS_RECOVERY_PROBE_STATUS
UdfsRecoveryPrepareRepair(const UDFS_RECOVERY_READER *reader,
                          const UDFS_RECOVERY_PROBE *probe,
                          const uint8_t recordingTime[12],
                          UDFS_RECOVERY_REPAIR_PLAN *plan)
{
    UDFS_RECOVERY_PROBE check;
    UDFS_RECOVERY_PROBE_STATUS status;
    UDFS_RECOVERY_SECTOR_EDIT *edit;
    uint8_t normalizedLvid[UDFS_RECOVERY_SECTOR_SIZE];
    const uint8_t *previousJournal = NULL;
    uint8_t repaired;
    uint32_t i, newPhysicalFree, newMetadataFree, crcLength, checksum;
    uint16_t crc;
    uint64_t nextUniqueId;

    if (!reader || !reader->ReadSector || !reader->Allocate ||
        !reader->Free || !probe || !recordingTime || !plan)
        return UdfsRecoveryProbeInvalidInput;
    memset(plan, 0, sizeof(*plan));
    if (!GraphCanRecover(probe)) return UdfsRecoveryProbeUnsupported;

    /* Rebuild from the current complete graph while collecting edits. A plan
     * never depends on a caller's truncated list of missing blocks. Exclusive
     * mount access must span probe, plan and apply. */
    status = ProbeLayout(reader, &check, plan);
    if (!GraphCanRecover(&check) ||
        RecoveryDifference(&check, probe, sizeof(check)) != 0) {
        if (status == UdfsRecoveryProbeValidLayout ||
            status == UdfsRecoveryProbeGraphMismatch ||
            status == UdfsRecoveryProbeLvidMismatch)
            status = UdfsRecoveryProbeCorrupt;
        goto done;
    }
    edit = &plan->LvidEdit;
    edit->Lba = probe->PrevailingLvidLba;
    status = ReadSector(reader, &check, edit->Lba, edit->Before);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    memcpy(normalizedLvid, edit->Before, UDFS_RECOVERY_SECTOR_SIZE);
    if (probe->LvidTagRepairMode == 2 ||
        probe->LvidTagRepairMode == 3 ||
        (probe->LvidTagRepairMode == 0 &&
         probe->JournalOpenLvidLba)) {
        plan->PrecedingClosedLvidLba = probe->PrecedingClosedLvidLba;
        if (plan->PrecedingClosedLvidLba >= reader->SectorCount ||
            plan->PrecedingClosedLvidLba == edit->Lba) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        status = ReadSector(reader, &check, plan->PrecedingClosedLvidLba,
                            plan->PrecedingClosedLvid);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
    }
    if (probe->LvidTagRepairMode == 3) {
        plan->JournalOpenLvidLba = probe->JournalOpenLvidLba;
        if (plan->JournalOpenLvidLba >= reader->SectorCount ||
            plan->JournalOpenLvidLba == edit->Lba ||
            plan->JournalOpenLvidLba == plan->PrecedingClosedLvidLba) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        status = ReadSector(reader, &check, plan->JournalOpenLvidLba,
                            plan->JournalOpenLvid);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        if (!JournalOpenValid(plan->JournalOpenLvid,
                              plan->JournalOpenLvidLba, edit->Lba,
                              plan->PrecedingClosedLvid) ||
            Read64(plan->JournalOpenLvid + 150) !=
                probe->JournalGeneration) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        plan->JournalNextLvidEdit.Lba =
            probe->JournalOtherLvidLba;
        if (plan->JournalNextLvidEdit.Lba >= reader->SectorCount ||
            plan->JournalNextLvidEdit.Lba == edit->Lba ||
            plan->JournalNextLvidEdit.Lba ==
                plan->JournalOpenLvidLba ||
            plan->JournalNextLvidEdit.Lba ==
                plan->PrecedingClosedLvidLba) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        status = ReadSector(reader, &check,
                            plan->JournalNextLvidEdit.Lba,
                            plan->JournalNextLvidEdit.Before);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        if (JournalOpenValid(plan->JournalNextLvidEdit.Before,
                             plan->JournalNextLvidEdit.Lba, edit->Lba,
                             plan->PrecedingClosedLvid) &&
            Read64(plan->JournalNextLvidEdit.Before + 150) + 1 ==
                probe->JournalGeneration)
            previousJournal = plan->JournalNextLvidEdit.Before;
        status = NormalizeLvidFromJournal(normalizedLvid,
                      edit->Lba, plan->JournalOpenLvid,
                      plan->JournalOpenLvidLba,
                      previousJournal, &repaired);
    } else if (probe->LvidTagRepairMode == 0 &&
               probe->JournalOpenLvidLba) {
        uint8_t expected[UDFS_RECOVERY_SECTOR_SIZE];
        uint32_t k;
        int alternateEmpty = 1;
        plan->JournalOpenLvidLba = probe->JournalOpenLvidLba;
        plan->JournalNextLvidEdit.Lba = probe->JournalOtherLvidLba;
        plan->JournalOtherDamaged = probe->JournalOtherDamaged;
        if (plan->JournalOpenLvidLba >= reader->SectorCount ||
            plan->JournalNextLvidEdit.Lba >= reader->SectorCount ||
            plan->JournalOpenLvidLba == edit->Lba ||
            plan->JournalNextLvidEdit.Lba == edit->Lba ||
            plan->JournalOpenLvidLba ==
                plan->JournalNextLvidEdit.Lba ||
            plan->JournalOpenLvidLba ==
                plan->PrecedingClosedLvidLba ||
            plan->JournalNextLvidEdit.Lba ==
                plan->PrecedingClosedLvidLba ||
            !probe->JournalGeneration ||
            probe->JournalGeneration == UINT64_MAX) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        status = ReadSector(reader, &check, plan->JournalOpenLvidLba,
                            plan->JournalOpenLvid);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        status = ReadSector(reader, &check,
                            plan->JournalNextLvidEdit.Lba,
                            plan->JournalNextLvidEdit.Before);
        if (status != UdfsRecoveryProbeValidLayout) goto done;
        if (!JournalOpenValid(plan->JournalOpenLvid,
                              plan->JournalOpenLvidLba, edit->Lba,
                              plan->PrecedingClosedLvid) ||
            Read64(plan->JournalOpenLvid + 150) !=
                probe->JournalGeneration) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        memcpy(expected, plan->JournalOpenLvid, sizeof(expected));
        RetagLvid(expected, edit->Lba);
        if (RecoveryDifference(normalizedLvid, expected,
                               sizeof(expected))) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        for (k = 0; k < UDFS_RECOVERY_SECTOR_SIZE; ++k)
            if (plan->JournalNextLvidEdit.Before[k]) {
                alternateEmpty = 0;
                break;
            }
        if ((alternateEmpty && probe->JournalOtherDamaged) ||
            (!alternateEmpty && !probe->JournalOtherDamaged &&
             (!JournalOpenValid(plan->JournalNextLvidEdit.Before,
                                plan->JournalNextLvidEdit.Lba, edit->Lba,
                                plan->PrecedingClosedLvid) ||
              Read64(plan->JournalNextLvidEdit.Before + 150) + 1 !=
                  probe->JournalGeneration))) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        status = NormalizeOpenLvidTag(normalizedLvid, edit->Lba, 0,
                                      NULL, &repaired);
    } else {
        status = NormalizeOpenLvidTag(normalizedLvid, edit->Lba,
                         probe->LvidTagRepairMode,
                         probe->LvidTagRepairMode == 2 ?
                             plan->PrecedingClosedLvid : NULL, &repaired);
    }
    if (status != UdfsRecoveryProbeValidLayout ||
        repaired != probe->LvidTagRepairMode) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }
    if ((probe->LvidTagRepairMode == 3 ?
            Read32(edit->Before + 28) > 1 :
            Read32(edit->Before + 28) != 0) ||
        Read32(normalizedLvid + 80) != probe->LvidPhysicalFreeBlocks ||
        Read32(normalizedLvid + 84) != probe->LvidMetadataFreeBlocks ||
        Read32(normalizedLvid + 88) != probe->LvidPhysicalSizeBlocks ||
        Read32(normalizedLvid + 92) != probe->LvidMetadataSizeBlocks ||
        Read64(normalizedLvid + 40) != probe->LvidNextUniqueId ||
        Read32(normalizedLvid + 128) != probe->LvidFileCount ||
        Read32(normalizedLvid + 132) != probe->LvidDirectoryCount) {
        status = UdfsRecoveryProbeCorrupt;
        goto done;
    }

    newPhysicalFree = probe->PhysicalFreeBlocks -
                      probe->ReferencedFreePhysicalBlocks +
                      (GraphCanRollbackPhysicalOrphans(probe) ?
                       probe->UnownedPhysicalBlocks : 0);
    newMetadataFree = probe->MetadataFreeBlocks -
                      probe->ReferencedFreeMetadataBlocks;
    nextUniqueId = probe->MaxUniqueId + 1;
    if (nextUniqueId < probe->LvidNextUniqueId)
        nextUniqueId = probe->LvidNextUniqueId;
    memcpy(edit->After, normalizedLvid, UDFS_RECOVERY_SECTOR_SIZE);
    /* A journaled OPEN descriptor already has an exact durable sidecar.
     * Changing only its final timestamp makes a torn CLOSED write differ
     * from that checkpoint, so the next mount cannot reconstruct it.
     * Preserve the OPEN time; a first legacy bootstrap below publishes its
     * new recording time in a sidecar before closing. */
    memcpy(edit->After + 16, normalizedLvid + 16, 12);
    Write32(edit->After + 28, 1);
    Write64(edit->After + 40, nextUniqueId);
    Write32(edit->After + 80, newPhysicalFree);
    Write32(edit->After + 84, newMetadataFree);
    Write32(edit->After + 92, probe->MetadataBlocks);
    Write32(edit->After + 128, probe->FilesVisited);
    Write32(edit->After + 132, probe->DirectoriesVisited);
    if (probe->LvidTagRepairMode == 3) {
        if ((Read32(plan->JournalOpenLvid + 162) == 1 &&
             (!probe->LvidCountersMatch ||
              probe->ReferencedFreePhysicalBlocks ||
              probe->ReferencedFreeMetadataBlocks ||
              plan->BitmapEditCount || plan->FeEditCount ||
              plan->FidEditCount ||
              nextUniqueId != probe->LvidNextUniqueId)) ||
            (Read32(plan->JournalOpenLvid + 162) == 0 &&
             probe->JournalGeneration == UINT64_MAX) ||
            probe->JournalOtherLvidLba >= reader->SectorCount ||
            probe->JournalOtherLvidLba == edit->Lba ||
            probe->JournalOtherLvidLba ==
                plan->JournalOpenLvidLba ||
            probe->JournalOtherLvidLba ==
                plan->PrecedingClosedLvidLba) {
            status = UdfsRecoveryProbeCorrupt;
            goto done;
        }
        Write64(edit->After + 150,
                probe->JournalGeneration +
                (Read32(plan->JournalOpenLvid + 162) == 0));
        Write32(edit->After + 162, 1);
    }
    status = PrepareLegacyJournalBootstrap(reader, probe,
                                           recordingTime, plan);
    if (status != UdfsRecoveryProbeValidLayout) goto done;
    if (probe->LvidTagRepairMode == 0 &&
        probe->JournalOpenLvidLba &&
        (probe->JournalOtherDamaged ||
         newPhysicalFree != probe->LvidPhysicalFreeBlocks ||
         newMetadataFree != probe->LvidMetadataFreeBlocks ||
         nextUniqueId != probe->LvidNextUniqueId ||
         probe->FilesVisited != probe->LvidFileCount ||
         probe->DirectoriesVisited != probe->LvidDirectoryCount)) {
        plan->JournalRepairRefresh = 1;
        Write64(edit->After + 150, probe->JournalGeneration + 1);
        Write32(edit->After + 162, 1);
    }
    crcLength = Read16(edit->After + 10);
    if (crcLength > UDFS_RECOVERY_SECTOR_SIZE - 16) {
        status = UdfsRecoveryProbeUnsupported;
        goto done;
    }
    crc = TagCrc(edit->After + 16, crcLength);
    edit->After[8] = (uint8_t)crc;
    edit->After[9] = (uint8_t)(crc >> 8);
    edit->After[4] = 0;
    checksum = 0;
    for (i = 0; i < 16; ++i)
        if (i != 4) checksum += edit->After[i];
    edit->After[4] = (uint8_t)checksum;
    status = ValidateTag(edit->After, 9, edit->Lba);
    if (status == UdfsRecoveryProbeValidLayout &&
        probe->LvidTagRepairMode == 3 &&
        Read32(plan->JournalOpenLvid + 162) == 0) {
        UDFS_RECOVERY_SECTOR_EDIT *journal = &plan->JournalNextLvidEdit;
        memcpy(journal->After, edit->After, UDFS_RECOVERY_SECTOR_SIZE);
        Write32(journal->After + 28, 0);
        Write32(journal->After + 162, 1); /* durable repair prepare */
        RetagLvid(journal->After, journal->Lba);
        if (!JournalOpenValid(journal->After, journal->Lba,
                              edit->Lba, plan->PrecedingClosedLvid))
            status = UdfsRecoveryProbeCorrupt;
    }
    if (status == UdfsRecoveryProbeValidLayout &&
        plan->JournalRepairRefresh) {
        UDFS_RECOVERY_SECTOR_EDIT *journal = &plan->JournalNextLvidEdit;
        memcpy(journal->After, edit->After, UDFS_RECOVERY_SECTOR_SIZE);
        Write32(journal->After + 28, 0);
        RetagLvid(journal->After, journal->Lba);
        if (!JournalOpenValid(journal->After, journal->Lba,
                              edit->Lba, plan->PrecedingClosedLvid) ||
            (plan->JournalOtherDamaged &&
             !TornInactiveJournalReplaceable(
                 journal->Before, journal->After,
                 plan->JournalOpenLvid, journal->Lba, edit->Lba,
                 plan->PrecedingClosedLvid, normalizedLvid)))
            status = UdfsRecoveryProbeCorrupt;
    }
    if (status == UdfsRecoveryProbeValidLayout &&
        plan->LegacyJournalBootstrap) {
        UDFS_RECOVERY_SECTOR_EDIT *journal = &plan->JournalNextLvidEdit;
        memcpy(journal->After, edit->After, UDFS_RECOVERY_SECTOR_SIZE);
        Write32(journal->After + 28, 0);
        RetagLvid(journal->After, journal->Lba);
        if (!JournalOpenValid(journal->After, journal->Lba,
                              edit->Lba, plan->PrecedingClosedLvid) ||
            !BootstrapSlotReplaceable(journal->Before,
                                      journal->After))
            status = UdfsRecoveryProbeCorrupt;
    }
    if (status == UdfsRecoveryProbeValidLayout) {
        plan->LvidTagRepairMode = repaired;
        plan->Ready = 1;
    }
done:
    if (status != UdfsRecoveryProbeValidLayout)
        memset(plan, 0, sizeof(*plan));
    return status;
}

UDFS_RECOVERY_PROBE_STATUS
UdfsRecoveryApplyRepair(const UDFS_RECOVERY_READER *reader,
                        const UDFS_RECOVERY_REPAIR_PLAN *plan,
                        UDFS_RECOVERY_PROBE *after)
{
    uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE];
    UDFS_RECOVERY_PROBE_STATUS status;
    uint32_t i, j;
    uint8_t repaired;

    if (!reader || !reader->ReadSector || !reader->WriteSector ||
        !reader->Flush || !reader->Allocate || !reader->Free ||
        !plan || !after || !plan->Ready ||
        plan->BitmapEditCount > UDFS_RECOVERY_MAX_BITMAP_EDITS ||
        plan->FeEditCount > 2 ||
        plan->FidEditCount > 1 ||
        plan->LvidEdit.Lba >= reader->SectorCount ||
        plan->LvidTagRepairMode > 3 ||
        plan->LegacyJournalBootstrap > 1 ||
        plan->JournalRepairRefresh > 1 ||
        plan->JournalOtherDamaged > 1 ||
        (plan->JournalOtherDamaged && !plan->JournalRepairRefresh) ||
        (plan->LegacyJournalBootstrap && plan->LvidTagRepairMode != 0) ||
        (plan->JournalRepairRefresh &&
         (plan->LvidTagRepairMode != 0 ||
          plan->LegacyJournalBootstrap)) ||
        ValidateTag(plan->LvidEdit.After, 9,
                    plan->LvidEdit.Lba) != UdfsRecoveryProbeValidLayout ||
        (plan->LvidTagRepairMode == 3 ?
            Read32(plan->LvidEdit.Before + 28) > 1 :
            Read32(plan->LvidEdit.Before + 28) != 0) ||
        Read32(plan->LvidEdit.After + 28) != 1)
        return UdfsRecoveryProbeInvalidInput;

    memset(after, 0, sizeof(*after));
    if (plan->LvidTagRepairMode == 2 ||
        plan->LvidTagRepairMode == 3 ||
        plan->LegacyJournalBootstrap ||
        plan->JournalRepairRefresh) {
        if (plan->PrecedingClosedLvidLba >= reader->SectorCount ||
            plan->PrecedingClosedLvidLba == plan->LvidEdit.Lba)
            return UdfsRecoveryProbeInvalidInput;
        status = ReadSector(reader, after,
                            plan->PrecedingClosedLvidLba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, plan->PrecedingClosedLvid,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
    }
    if (plan->LvidTagRepairMode == 3) {
        const UDFS_RECOVERY_SECTOR_EDIT *journal =
            &plan->JournalNextLvidEdit;
        int recoveryReady =
            Read32(plan->JournalOpenLvid + 162) == 1;
        if (plan->JournalOpenLvidLba >= reader->SectorCount ||
            plan->JournalOpenLvidLba == plan->LvidEdit.Lba ||
            plan->JournalOpenLvidLba ==
                plan->PrecedingClosedLvidLba ||
            journal->Lba >= reader->SectorCount ||
            journal->Lba == plan->LvidEdit.Lba ||
            journal->Lba == plan->JournalOpenLvidLba ||
            journal->Lba == plan->PrecedingClosedLvidLba ||
            !JournalOpenValid(plan->JournalOpenLvid,
                              plan->JournalOpenLvidLba,
                              plan->LvidEdit.Lba,
                              plan->PrecedingClosedLvid) ||
            (!recoveryReady &&
             (!JournalOpenValid(journal->After, journal->Lba,
                                plan->LvidEdit.Lba,
                                plan->PrecedingClosedLvid) ||
              Read64(plan->JournalOpenLvid + 150) == UINT64_MAX ||
              Read64(journal->After + 150) !=
                  Read64(plan->JournalOpenLvid + 150) + 1)) ||
            Read64(plan->LvidEdit.After + 150) !=
                Read64(plan->JournalOpenLvid + 150) +
                    !recoveryReady ||
            Read32(plan->LvidEdit.After + 162) != 1)
            return UdfsRecoveryProbeInvalidInput;
        memcpy(sector,
               recoveryReady ? plan->JournalOpenLvid : journal->After,
               UDFS_RECOVERY_SECTOR_SIZE);
        Write32(sector + 28, 1);
        RetagLvid(sector, plan->LvidEdit.Lba);
        if (RecoveryDifference(sector, plan->LvidEdit.After,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeInvalidInput;
        status = ReadSector(reader, after,
                            plan->JournalOpenLvidLba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, plan->JournalOpenLvid,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
        status = ReadSector(reader, after, journal->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, journal->Before,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
    }
    if (plan->JournalRepairRefresh) {
        const UDFS_RECOVERY_SECTOR_EDIT *journal =
            &plan->JournalNextLvidEdit;
        uint32_t k;
        int alternateEmpty = 1;
        if (plan->JournalOpenLvidLba >= reader->SectorCount ||
            plan->JournalOpenLvidLba == plan->LvidEdit.Lba ||
            plan->JournalOpenLvidLba ==
                plan->PrecedingClosedLvidLba ||
            journal->Lba >= reader->SectorCount ||
            journal->Lba == plan->LvidEdit.Lba ||
            journal->Lba == plan->JournalOpenLvidLba ||
            journal->Lba == plan->PrecedingClosedLvidLba ||
            !JournalOpenValid(plan->JournalOpenLvid,
                              plan->JournalOpenLvidLba,
                              plan->LvidEdit.Lba,
                              plan->PrecedingClosedLvid) ||
            Read64(plan->JournalOpenLvid + 150) == UINT64_MAX ||
            !JournalOpenValid(journal->After, journal->Lba,
                              plan->LvidEdit.Lba,
                              plan->PrecedingClosedLvid) ||
            Read64(journal->After + 150) !=
                Read64(plan->JournalOpenLvid + 150) + 1 ||
            Read32(journal->After + 162) != 1 ||
            Read64(plan->LvidEdit.After + 150) !=
                Read64(journal->After + 150) ||
            Read32(plan->LvidEdit.After + 162) != 1)
            return UdfsRecoveryProbeInvalidInput;
        memcpy(sector, plan->JournalOpenLvid,
               UDFS_RECOVERY_SECTOR_SIZE);
        RetagLvid(sector, plan->LvidEdit.Lba);
        if (RecoveryDifference(sector, plan->LvidEdit.Before,
                               UDFS_RECOVERY_SECTOR_SIZE))
            return UdfsRecoveryProbeInvalidInput;
        memcpy(sector, journal->After, UDFS_RECOVERY_SECTOR_SIZE);
        Write32(sector + 28, 1);
        RetagLvid(sector, plan->LvidEdit.Lba);
        if (RecoveryDifference(sector, plan->LvidEdit.After,
                               UDFS_RECOVERY_SECTOR_SIZE))
            return UdfsRecoveryProbeInvalidInput;
        for (k = 0; k < UDFS_RECOVERY_SECTOR_SIZE; ++k)
            if (journal->Before[k]) {
                alternateEmpty = 0;
                break;
            }
        if ((alternateEmpty && plan->JournalOtherDamaged) ||
            (!alternateEmpty && !plan->JournalOtherDamaged &&
             (!JournalOpenValid(journal->Before, journal->Lba,
                                plan->LvidEdit.Lba,
                                plan->PrecedingClosedLvid) ||
              Read64(journal->Before + 150) + 1 !=
                  Read64(plan->JournalOpenLvid + 150))) ||
            (plan->JournalOtherDamaged &&
             !TornInactiveJournalReplaceable(
                 journal->Before, journal->After,
                 plan->JournalOpenLvid, journal->Lba,
                 plan->LvidEdit.Lba, plan->PrecedingClosedLvid,
                 sector)))
            return UdfsRecoveryProbeInvalidInput;
        status = ReadSector(reader, after,
                            plan->JournalOpenLvidLba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, plan->JournalOpenLvid,
                               UDFS_RECOVERY_SECTOR_SIZE))
            return UdfsRecoveryProbeCorrupt;
        status = ReadSector(reader, after, journal->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, journal->Before,
                               UDFS_RECOVERY_SECTOR_SIZE))
            return UdfsRecoveryProbeCorrupt;
    }
    if (plan->LegacyJournalBootstrap) {
        const UDFS_RECOVERY_SECTOR_EDIT *journal =
            &plan->JournalNextLvidEdit;
        if (journal->Lba >= reader->SectorCount - 1 ||
            journal->Lba == plan->LvidEdit.Lba ||
            journal->Lba + 1 == plan->LvidEdit.Lba ||
            plan->PrecedingClosedLvidLba + 1 !=
                plan->LvidEdit.Lba ||
            Read32(plan->PrecedingClosedLvid + 28) != 0 ||
            !JournalOpenValid(journal->After, journal->Lba,
                              plan->LvidEdit.Lba,
                              plan->PrecedingClosedLvid) ||
            Read64(journal->After + 150) != 1 ||
            Read32(journal->After + 162) != 1)
            return UdfsRecoveryProbeInvalidInput;
        memcpy(sector, journal->After, UDFS_RECOVERY_SECTOR_SIZE);
        Write32(sector + 28, 1);
        RetagLvid(sector, plan->LvidEdit.Lba);
        if (RecoveryDifference(sector, plan->LvidEdit.After,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeInvalidInput;
        status = ReadSector(reader, after, journal->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, journal->Before,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
        status = ReadSector(reader, after, journal->Lba + 1, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        for (i = 0; i < UDFS_RECOVERY_SECTOR_SIZE; ++i)
            if (sector[i]) return UdfsRecoveryProbeCorrupt;
    }
    memcpy(sector, plan->LvidEdit.Before, UDFS_RECOVERY_SECTOR_SIZE);
    if (plan->LvidTagRepairMode == 3) {
        const uint8_t *previousJournal = NULL;
        if (JournalOpenValid(plan->JournalNextLvidEdit.Before,
                             plan->JournalNextLvidEdit.Lba,
                             plan->LvidEdit.Lba,
                             plan->PrecedingClosedLvid) &&
            Read64(plan->JournalNextLvidEdit.Before + 150) + 1 ==
                Read64(plan->JournalOpenLvid + 150))
            previousJournal = plan->JournalNextLvidEdit.Before;
        status = NormalizeLvidFromJournal(sector,
                    plan->LvidEdit.Lba, plan->JournalOpenLvid,
                    plan->JournalOpenLvidLba, previousJournal, &repaired);
    } else {
        status = NormalizeOpenLvidTag(sector, plan->LvidEdit.Lba,
                         plan->LvidTagRepairMode,
                         plan->LvidTagRepairMode == 2 ?
                             plan->PrecedingClosedLvid : NULL, &repaired);
    }
    if (status != UdfsRecoveryProbeValidLayout ||
        repaired != plan->LvidTagRepairMode)
        return UdfsRecoveryProbeInvalidInput;

    /* Verify the complete plan before the first mutation. A stale plan must
     * be regenerated from a fresh ownership graph. */
    for (i = 0; i < plan->BitmapEditCount; ++i) {
        const UDFS_RECOVERY_SECTOR_EDIT *edit = &plan->BitmapEdits[i];
        if (edit->Lba >= reader->SectorCount ||
            edit->Lba == plan->LvidEdit.Lba ||
            RecoveryDifference(edit->Before, edit->After,
                   UDFS_RECOVERY_SECTOR_SIZE) == 0)
            return UdfsRecoveryProbeInvalidInput;
        for (j = 0; j < i; ++j)
            if (edit->Lba == plan->BitmapEdits[j].Lba)
                return UdfsRecoveryProbeInvalidInput;
        status = ReadSector(reader, after, edit->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, edit->Before,
                   UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
    }
    for (i = 0; i < plan->FeEditCount; ++i) {
        const UDFS_RECOVERY_SECTOR_EDIT *edit = &plan->FeEdits[i];
        uint16_t kind = Read16(edit->Before);
        if (edit->Lba >= reader->SectorCount ||
            edit->Lba == plan->LvidEdit.Lba ||
            (kind != 261 && kind != 266) ||
            ValidateTag(edit->Before, kind,
                Read32(edit->Before + 12)) !=
                    UdfsRecoveryProbeValidLayout ||
            ValidateTag(edit->After, kind,
                Read32(edit->Before + 12)) !=
                    UdfsRecoveryProbeValidLayout ||
            !DirectoryFePairDiffersOnlyInLinkCount(
                edit->Before, edit->After,
                Read32(edit->Before + 12)))
            return UdfsRecoveryProbeInvalidInput;
        for (j = 0; j < plan->BitmapEditCount; ++j)
            if (edit->Lba == plan->BitmapEdits[j].Lba)
                return UdfsRecoveryProbeInvalidInput;
        for (j = 0; j < i; ++j)
            if (edit->Lba == plan->FeEdits[j].Lba)
                return UdfsRecoveryProbeInvalidInput;
        status = ReadSector(reader, after, edit->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, edit->Before,
                   UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
    }
    if (plan->FidEditCount) {
        const UDFS_RECOVERY_SECTOR_EDIT *edit = &plan->FidEdit;
        int liveOnMirror;
        uint32_t child, recordOffset;
        if (edit->Lba >= reader->SectorCount ||
            plan->FidLiveLba >= reader->SectorCount ||
            edit->Lba == plan->FidLiveLba ||
            edit->Lba == plan->LvidEdit.Lba ||
            !DirectoryFidPairLiveDeleted(
                edit->Before, edit->After,
                plan->FidEditLogicalLbn,
                &liveOnMirror, &child, &recordOffset) ||
            liveOnMirror != 1)
            return UdfsRecoveryProbeInvalidInput;
        for (j = 0; j < plan->BitmapEditCount; ++j)
            if (edit->Lba == plan->BitmapEdits[j].Lba ||
                plan->FidLiveLba ==
                    plan->BitmapEdits[j].Lba)
                return UdfsRecoveryProbeInvalidInput;
        for (j = 0; j < plan->FeEditCount; ++j)
            if (edit->Lba == plan->FeEdits[j].Lba ||
                plan->FidLiveLba == plan->FeEdits[j].Lba)
                return UdfsRecoveryProbeInvalidInput;
        status = ReadSector(reader, after, edit->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, edit->Before,
                   UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
        status = ReadSector(reader, after, plan->FidLiveLba,
                            sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, edit->After,
                   UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
    }
    status = ReadSector(reader, after, plan->LvidEdit.Lba, sector);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    if (RecoveryDifference(sector, plan->LvidEdit.Before,
               UDFS_RECOVERY_SECTOR_SIZE) != 0)
        return UdfsRecoveryProbeCorrupt;

    /* A split tombstone never commits the delete: restore the valid live
     * FID first.  All later states then retain a reachable child whose
     * pre-freed bit and parent count can be reconciled on retry. */
    if (plan->FidEditCount) {
        if (reader->WriteSector(reader->Context,
                                plan->FidEdit.Lba,
                                plan->FidEdit.After) != 0 ||
            reader->Flush(reader->Context) != 0)
            return UdfsRecoveryProbeIoError;
        status = ReadSector(reader, after,
                            plan->FidEdit.Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, plan->FidEdit.After,
                   UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeIoError;
    }
    /* Repair the parent's link count while the child's pre-free bit is still
     * present. That bit is the evidence needed to retry an interrupted FE
     * repair; clearing it first would leave an unexplained low link count.
     * An OPEN LVID blocks allocation throughout this recovery sequence. */
    for (i = 0; i < plan->FeEditCount; ++i)
        if (reader->WriteSector(reader->Context,
                                plan->FeEdits[i].Lba,
                                plan->FeEdits[i].After) != 0)
            return UdfsRecoveryProbeIoError;
    if (plan->FeEditCount && reader->Flush(reader->Context) != 0)
        return UdfsRecoveryProbeIoError;
    for (i = 0; i < plan->FeEditCount; ++i) {
        status = ReadSector(reader, after, plan->FeEdits[i].Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, plan->FeEdits[i].After,
                   UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeIoError;
    }
    for (i = 0; i < plan->BitmapEditCount; ++i)
        if (reader->WriteSector(reader->Context,
                                plan->BitmapEdits[i].Lba,
                                plan->BitmapEdits[i].After) != 0)
            return UdfsRecoveryProbeIoError;
    /* Flush is required even for zero bitmap edits: a prior interrupted
     * attempt may have left the newly allocated bits in a device cache. */
    if (reader->Flush(reader->Context) != 0)
        return UdfsRecoveryProbeIoError;
    for (i = 0; i < plan->BitmapEditCount; ++i) {
        status = ReadSector(reader, after,
                            plan->BitmapEdits[i].Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, plan->BitmapEdits[i].After,
                   UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeIoError;
    }

    /* Bitmap writes must be durable AND the entire owner graph must close
     * before publishing CLOSED. Check the planned aggregate values against a
     * new OPEN probe, including file/directory counts and the ID high-water
     * mark. Interrupted bitmap batches remain OPEN and can be replanned. */
    status = UdfsRecoveryProbeLayout(reader, after);
    if ((status != UdfsRecoveryProbeValidLayout &&
         status != UdfsRecoveryProbeLvidMismatch) ||
        !GraphCanRecover(after) || !after->CompleteOwnershipGraph ||
        after->PrevailingLvidLba != plan->LvidEdit.Lba ||
        after->PhysicalFreeBlocks != Read32(plan->LvidEdit.After + 80) ||
        after->MetadataFreeBlocks != Read32(plan->LvidEdit.After + 84) ||
        after->PartitionBlocks != Read32(plan->LvidEdit.After + 88) ||
        after->MetadataBlocks != Read32(plan->LvidEdit.After + 92) ||
        after->FilesVisited != Read32(plan->LvidEdit.After + 128) ||
        after->DirectoriesVisited != Read32(plan->LvidEdit.After + 132) ||
        after->MaxUniqueId >= Read64(plan->LvidEdit.After + 40))
        return status == UdfsRecoveryProbeValidLayout ||
               status == UdfsRecoveryProbeLvidMismatch ?
               UdfsRecoveryProbeCorrupt : status;

    if (plan->LegacyJournalBootstrap) {
        const UDFS_RECOVERY_SECTOR_EDIT *journal =
            &plan->JournalNextLvidEdit;
        /* Publish the exact recovery result as an OPEN sidecar before the
         * first CLOSED write on this old volume. A subsequent crash may
         * then retry from the sidecar without guessing lost LVID bytes. */
        status = ReadSector(reader, after, journal->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, journal->Before,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
        if (reader->WriteSector(reader->Context, journal->Lba,
                                journal->After) != 0 ||
            reader->Flush(reader->Context) != 0)
            return UdfsRecoveryProbeIoError;
        status = ReadSector(reader, after, journal->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, journal->After,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeIoError;
        status = UdfsRecoveryProbeLayout(reader, after);
        if ((status != UdfsRecoveryProbeValidLayout &&
             status != UdfsRecoveryProbeLvidMismatch) ||
            !GraphCanRecover(after) || !after->CompleteOwnershipGraph ||
            after->PrevailingLvidLba != plan->LvidEdit.Lba ||
            after->JournalOpenLvidLba != journal->Lba ||
            after->JournalGeneration != 1 ||
            after->LvidTagRepairMode != 3 ||
            after->FilesVisited !=
                Read32(plan->LvidEdit.After + 128) ||
            after->DirectoriesVisited !=
                Read32(plan->LvidEdit.After + 132) ||
            after->PhysicalFreeBlocks !=
                Read32(plan->LvidEdit.After + 80) ||
            after->MetadataFreeBlocks !=
                Read32(plan->LvidEdit.After + 84) ||
            after->MaxUniqueId >=
                Read64(plan->LvidEdit.After + 40))
            return status == UdfsRecoveryProbeValidLayout ||
                   status == UdfsRecoveryProbeLvidMismatch ?
                   UdfsRecoveryProbeCorrupt : status;
    }

    if ((plan->LvidTagRepairMode == 3 &&
         Read32(plan->JournalOpenLvid + 162) == 0) ||
        plan->JournalRepairRefresh) {
        const UDFS_RECOVERY_SECTOR_EDIT *journal =
            &plan->JournalNextLvidEdit;
        /* Preserve the previously durable OPEN slot until the alternate
         * slot has reached stable media. A torn journal update therefore
         * leaves at least one valid checkpoint for the next cold retry. */
        status = ReadSector(reader, after, journal->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, journal->Before,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeCorrupt;
        if (reader->WriteSector(reader->Context, journal->Lba,
                                journal->After) != 0 ||
            reader->Flush(reader->Context) != 0)
            return UdfsRecoveryProbeIoError;
        status = ReadSector(reader, after, journal->Lba, sector);
        if (status != UdfsRecoveryProbeValidLayout) return status;
        if (RecoveryDifference(sector, journal->After,
                               UDFS_RECOVERY_SECTOR_SIZE) != 0)
            return UdfsRecoveryProbeIoError;
        status = UdfsRecoveryProbeLayout(reader, after);
        if ((status != UdfsRecoveryProbeValidLayout &&
             status != UdfsRecoveryProbeLvidMismatch) ||
            !GraphCanRecover(after) || !after->CompleteOwnershipGraph ||
            after->JournalOpenLvidLba != journal->Lba ||
            after->JournalGeneration !=
                Read64(journal->After + 150) ||
            after->FilesVisited !=
                Read32(plan->LvidEdit.After + 128) ||
            after->DirectoriesVisited !=
                Read32(plan->LvidEdit.After + 132) ||
            after->PhysicalFreeBlocks !=
                Read32(plan->LvidEdit.After + 80) ||
            after->MetadataFreeBlocks !=
                Read32(plan->LvidEdit.After + 84))
            return status == UdfsRecoveryProbeValidLayout ||
                   status == UdfsRecoveryProbeLvidMismatch ?
                   UdfsRecoveryProbeCorrupt : status;
    }

    status = ReadSector(reader, after, plan->LvidEdit.Lba, sector);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    if (RecoveryDifference(sector, plan->LvidEdit.Before,
               UDFS_RECOVERY_SECTOR_SIZE) != 0)
        return UdfsRecoveryProbeCorrupt;
    if (reader->WriteSector(reader->Context, plan->LvidEdit.Lba,
                            plan->LvidEdit.After) != 0 ||
        reader->Flush(reader->Context) != 0)
        return UdfsRecoveryProbeIoError;
    status = ReadSector(reader, after, plan->LvidEdit.Lba, sector);
    if (status != UdfsRecoveryProbeValidLayout) return status;
    if (RecoveryDifference(sector, plan->LvidEdit.After,
               UDFS_RECOVERY_SECTOR_SIZE) != 0)
        return UdfsRecoveryProbeIoError;

    status = UdfsRecoveryProbeLayout(reader, after);
    if (status != UdfsRecoveryProbeValidLayout ||
        !after->CompleteOwnershipGraph || !after->LvidCountersMatch ||
        after->LvidIntegrityType != 1 || after->WritableMountAuthorized)
        return status == UdfsRecoveryProbeValidLayout ?
               UdfsRecoveryProbeCorrupt : status;
    return UdfsRecoveryProbeValidLayout;
}
