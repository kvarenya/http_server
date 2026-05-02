# ──────────────────────────────────────────────
# Makefile — tiny-httpd
# ──────────────────────────────────────────────

CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 \
          -Iinclude \
          -g                    # swap -g for -O2 in production
LDFLAGS =

TARGET  = build/httpd
SRCS    = src/main.c \
          src/server.c \
          src/request.c \
          src/response.c \
          src/router.c
OBJS    = $(SRCS:src/%.c=build/%.o)

# ── default ──────────────────────────────────
.PHONY: all
all: build $(TARGET)

build:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "  LD  $@"

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "  CC  $<"

# ── helpers ──────────────────────────────────
.PHONY: run
run: all
	./$(TARGET)

# Run on a custom port:  make run PORT=9090
PORT ?= 8080
.PHONY: runport
runport: all
	./$(TARGET) $(PORT)

.PHONY: clean
clean:
	rm -rf build

# Quick smoke-test with curl
.PHONY: test
test:
	@echo "── GET  /         ──────────────────"
	@curl -s http://localhost:$(PORT)/         | python3 -m json.tool || true
	@echo "── GET  /health   ──────────────────"
	@curl -s http://localhost:$(PORT)/health   | python3 -m json.tool || true
	@echo "── GET  /users/42 ──────────────────"
	@curl -s http://localhost:$(PORT)/users/42 | python3 -m json.tool || true
	@echo "── POST /users    ──────────────────"
	@curl -s -X POST http://localhost:$(PORT)/users \
	     -H "Content-Type: application/json" \
	     -d '{"name":"Bob"}' | python3 -m json.tool || true
	@echo "── POST /echo     ──────────────────"
	@curl -s -X POST http://localhost:$(PORT)/echo -d "ping"
	@echo ""
	@echo "── GET  /missing  (404) ─────────────"
	@curl -s http://localhost:$(PORT)/missing  | python3 -m json.tool || true

.PHONY: help
help:
	@echo "Targets:"
	@echo "  make          — build"
	@echo "  make run      — build & run on port 8080"
	@echo "  make runport PORT=9090"
	@echo "  make test     — smoke-test (server must be running)"
	@echo "  make clean    — remove build artifacts"
