
INCOPTS=-I/opt/local/include/ -I/usr/local/include
LDOPTS=-L/opt/local/lib/ -L/usr/local/lib/
LDLIBS=-lcurl -lidn -lssl -lcrypto -lz -lfolly -lcurlpp -lglog
LDLIBS+=-ldouble-conversion -lpython2.7 -lid3 -lid3tag

comp: snatch.cpp
	c++ -std=c++11 $(INCOPTS) $(LDOPTS) $(LDLIBS) snatch.cpp -o snatch

clean:
	rm snatch
	rm download.pyc
