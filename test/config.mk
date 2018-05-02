# paths
TEST_DIR := test

# flags
CFLAGS_TEST := $(CFLAGS) -I${SRC_DIR} $(shell pkg-config --cflags check)
LDFLAGS_TEST := $(LDFLAGS) $(shell pkg-config --libs check)

# files needed to generate test suite
TEST_FILES := test/core.c \
              test/utils-test.c

SRC_FILES := src/utils.c \
             src/conf.c \
             src/log.c

# test target
TEST := run_test

$(TEST_DIR)/test.c:
	@checkmk clean_mode=1 $(TEST_FILES) > $@

test: $(TEST_DIR)/test.c
	@$(CC) $(CFLAGS_TEST) -o $(TEST) $^ $(SRC_FILES) $(LDFLAGS_TEST)
	@rm -f $^
	@echo "\n=============================================="
	@echo "============ Start the Test Suite ============\n"
	@CK_VERBOSITY=normal ./$(TEST)
	@echo "\n=============================================="
	@rm -f $(TEST)
