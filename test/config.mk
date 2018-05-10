# paths
TEST_DIR := test

# cflags
CFLAGS_TEST := $(CFLAGS)
CFLAGS_TEST +=  $(shell pkg-config --cflags check)
CFLAGS_TEST += -D ROOT_DIR='"${ROOT_DIR}"'

# ldflags
LDFLAGS_TEST := $(LDFLAGS)
LDFLAGS_TEST +=  $(shell pkg-config --libs check)

# source and objects needed to generate test suite
TEST_SRC := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ := $(TEST_SRC:.c=.o)

DEPS_SRC := src/utils.c \
            src/conf.c \
            src/log.c
DEPS_OBJ := $(patsubst $(SRC_DIR)/%.c, $(TEST_DIR)/%.o, $(DEPS_SRC))

# test target
TEST := run_test

$(TEST_DIR)/%.o: $(SRC_DIR)/%.c
	@${CC} -o $@ -c $< $(CFLAGS_TEST)

$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@${CC} -o $@ -c $< $(CFLAGS_TEST)

test: $(TEST_OBJ) $(DEPS_OBJ)
	@$(CC) $(CFLAGS_TEST) -o $(TEST) $^ $(LDFLAGS_TEST)
	@rm -f $^
	@./$(TEST)
	@rm -f $(TEST)
