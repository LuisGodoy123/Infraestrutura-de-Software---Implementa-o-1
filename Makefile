CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = processflow
SRC = processflow.c

all: $(TARGET)

$(TARGET): $(SRC) processflow.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

test: all
	@if [ -d tests ]; then \
		for arquivo in tests/*.pf; do \
			echo "== $$arquivo =="; \
			./$(TARGET) $$arquivo; \
		done; \
	else \
		echo "nenhum teste em tests/"; \
	fi

.PHONY: all clean test
