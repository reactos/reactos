boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat.h: $(OUTPUT_)boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat.o $(OUTPUT_)boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat32.o
	$(BIN2C_TARGET) $(OUTPUT_)boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat.o boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat.h fat_data

boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat32.h:
	$(BIN2C_TARGET) $(OUTPUT_)boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat32.o boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat32.h fat32_data

bootsector_headers: boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat.h boot$(SEP)freeldr$(SEP)bootsect$(SEP)fat32.h
