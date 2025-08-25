CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -fPIC -D_POSIX_C_SOURCE=199309L
LDFLAGS = -lm

# Library
LIB_NAME = libspen_adapter
LIB_STATIC = $(LIB_NAME).a
LIB_SHARED = $(LIB_NAME).so

# Sources
SOURCES = spen_adapter.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = spen_adapter.h

# Test harness
TEST_TARGET = spen_test_harness
TEST_SOURCES = spen_test_harness.c
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

.PHONY: all clean test install

all: $(LIB_STATIC) $(LIB_SHARED) $(TEST_TARGET)

# Static library
$(LIB_STATIC): $(OBJECTS)
	$(AR) rcs $@ $^

# Shared library  
$(LIB_SHARED): $(OBJECTS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# Object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Test harness
$(TEST_TARGET): $(TEST_OBJECTS) $(LIB_STATIC)
	$(CC) -o $@ $^ $(LDFLAGS)

# Test harness objects
$(TEST_OBJECTS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

install: $(LIB_STATIC) $(LIB_SHARED) $(HEADERS)
	install -d $(DESTDIR)/usr/local/lib
	install -d $(DESTDIR)/usr/local/include
	install $(LIB_STATIC) $(DESTDIR)/usr/local/lib/
	install $(LIB_SHARED) $(DESTDIR)/usr/local/lib/
	install $(HEADERS) $(DESTDIR)/usr/local/include/

clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(LIB_STATIC) $(LIB_SHARED) $(TEST_TARGET)

# Format code (requires clang-format)
format:
	clang-format -i *.c *.h

# Lint code (requires clang-tidy)  
lint:
	clang-tidy *.c -- $(CFLAGS)