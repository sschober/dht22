BIN=dht22
$(BIN): $(BIN).c
	gcc -o $@ $< -lwiringPi
clean:
	rm $(BIN)
