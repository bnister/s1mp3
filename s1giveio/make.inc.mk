GIFPREF=s1giveio/
GIDPREF=$(GIFPREF)dasm

$(GIDPREF).o: $(GIDPREF).cpp $(GIDPREF).h common/common.h
	$(CC) -c $(CCFLAGS) -o $@ $(GIDPREF).cpp

$(GIFPREF)s1giveio: $(GIFPREF)main.cpp $(GIDPREF).h $(GIDPREF).o common/common.h common/GiveIO.h common/GiveIO.o common/AdfuSession.h common/AdfuSession.o
	$(CC) $(CCFLAGS) -o $@ $(GIFPREF)main.cpp common/AdfuSession.o common/GiveIO.o $(GIDPREF).o $(LDFLAGS)

clean_s1giveio:
	rm -f $(GIDPREF).o
	rm -f $(GIFPREF)s1giveio
