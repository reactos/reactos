/START OF 32-BIT CODE/,/ELSE/{
s/\.data/\.fardata INSTDATA\
        db "Smag"               ; junk to give a non-zero address /
}
/; IS_32/,/^END/{
s/\.data/\.fardata INSTDATA\
        db "Smag"               ; junk to give a non-zero address /
}
