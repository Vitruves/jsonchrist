CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -O2 -std=c11
LDFLAGS =
INCLUDES = -Isrc

SRCDIR = src
OBJDIR = obj

SRCS = src/json_format.c src/json_parser.c src/json_stats.c src/jsonchrist.c
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = jsonchrist

.PHONY: all clean dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJDIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

test: $(TARGET)
	./$(TARGET) --validate test.json
	./$(TARGET) --tree test.json
	./$(TARGET) --pretty --indent 2 test.json
	./$(TARGET) --stats test.json 