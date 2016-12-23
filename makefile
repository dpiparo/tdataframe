TARGETS:=tdf001_introduction tdf002_dataModel test2

all: $(TARGETS)

%: %.cxx TDataFrame.hxx; \
   g++ -std=c++11 -o $@ $< `root-config --libs --cflags` -lTreePlayer

.PHONY: clean
clean: ;\
   rm -rf $(TARGETS)
