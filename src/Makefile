################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for Recitation 1.             #
#                                                                              #
# Authors:   Lincong Li 							                           #
#                                                                              #
################################################################################

define build_obj
$(CC) $(CFLAGS) -c  $< -o $@
endef


SERVER_SRC = server.c utility.c message.c connection_handlers.c
OBJ_DIR=objs
PARSE_DIR=$(SOURCE_DIR)/parser
PARSER_DEPS = $(PARSE_DIR)/parse.h $(PARSE_DIR)/y.tab.h

CC=gcc
CFLAGS=-Wall

# add server objects
OBJ=$(OBJ_DIR)/server.o
OBJ+=$(OBJ_DIR)/utility.o
OBJ+=$(OBJ_DIR)/message.o
OBJ+=$(OBJ_DIR)/connection_handlers.o

# add parser objects
OBJ+=$(OBJ_DIR)/y.tab.o
OBJ+=$(OBJ_DIR)/lex.yy.o
OBJ+=$(OBJ_DIR)/parse.o

default: all

all: pre server

pre:
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/server.o: server.c
	$(build_obj)

$(OBJ_DIR)/utility.o: utility.c
	$(build_obj)

$(OBJ_DIR)/message.o: message.c
	$(build_obj)

$(OBJ_DIR)/connection_handlers.o: connection_handlers.c
	$(build_obj)

$(PARSE_DIR)/lex.yy.c: $(PARSE_DIR)/lexer.l
	flex -o $@ $^

$(PARSE_DIR)/y.tab.c: $(PARSE_DIR)/parser.y
	yacc -d $^
	mv y.tab.* $(PARSE_DIR)/

$(OBJ_DIR)/%.o: $(PARSE_DIR)/%.c $(PARSER_DEPS)
	  $(build_obj)

$(OBJ_DIR)/lex.yy.o: $(PARSE_DIR)/lex.yy.c $(PARSER_DEPS)
	  $(build_obj)

$(OBJ_DIR)/y.tab.o: $(PARSE_DIR)/y.tab.c $(PARSER_DEPS)
	  $(build_obj)

server: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm $(OBJ_DIR)/*.o
	rm $(PARSE_DIR)/y.tab.c $(PARSE_DIR)/lex.yy.c
	rm server
