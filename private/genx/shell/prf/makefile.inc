
#
# Inference Rules
#
.SUFFIXES:.prf

.prf{$(PRIVATE_PRF_PATH)}.prf:
    copy $** $@

clean:
    -del /F /Q $(NTTARGETFILE0)
