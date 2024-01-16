all: scatter scatter2 stattests 

bin10k.txt:  rngdmp.10k.csv
	cat rngdmp.10k.csv | cut -f1 -d: > bin10k.txt

bin10k.bin: bin10k.txt
	./conv4Bits.py ./bin10k.txt ./bin10k.bin

scatter: bin10k.bin
	./scatter2.py  ./bin10k.bin

scatter2: bin10k.bin
	./scatter.py ./bin10k.bin

bin250k.txt: rngdmp.250k.csv
	cat rngdmp.250k.csv | cut -f1 -d: > bin250k.txt

bin250k.bin: bin250k.txt
	./conv4Bits.py ./bin250k.txt ./bin250k.bin

stattests: bin250k.bin
	ent bin250k.bin > stattests.report.txt


clean:
	rm -f ./bin10k.txt ./bin10k.bin ./stattests.report.txt
