CC := gcc
SRC := voc.c
TGT ?= voc
CFLAGS +=

$(TGT) : $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

install_extra : $(TGT)
	install -m 755 $(TGT) $(EXTRA_RUN)/
