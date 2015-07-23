
##
# Target name.
#
TARGET = lela

##
# Target static library.
#
TARGET_STATIC ?= lib$(TARGET).a

##
# Main program.
#
MAIN ?= src/main.c

##
# Source files.
#
SRC := $(filter-out $(MAIN), $(wildcard src/*.c)) $(wildcard deps/*/*.c)

##
# Object files.
#
OBJS := $(SRC:.c=.o)

##
# SQL files
#

SQL := $(wildcard src/sql/*.sql)

##
# SQL to header files
#

SQL_H := $(SQL:.sql=.h)

##
# Compiler flags
#
CFLAGS += -std=c99
CFLAGS += -Ideps
CFLAGS += -Wall

define TARGET_HEADER_FILE_NAME
$(shell echo $(@:.h=) | xargs basename)
endef

.SUFFIXES: .sql

default: $(SQL_H) $(TARGET)

##
# Build program target.
#
$(TARGET): $(TARGET_STATIC)
	$(CC) $(CFLAGS) $(TARGET_STATIC) $(MAIN) -o $(@)

##
# Build target static library.
#
$(TARGET_STATIC): $(OBJS)
	$(AR) crus $@ $(OBJS)

##
# Build object file.
#
%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

##
# Convert SQL files to header files.
#
src/sql/%.h: src/sql/%.sql
	@echo "Building $@ from $^"
	@rm -f $@
	@echo "// DO NOT EDIT - This file was automatically generated '`date`'" >> $@
	@echo "#ifndef lela_sql_$(TARGET_HEADER_FILE_NAME)_h" >> $@
	@echo "#define lela_sql_$(TARGET_HEADER_FILE_NAME)_h 1" >> $@
	@echo "#define lela_sql_$(TARGET_HEADER_FILE_NAME)() \\" >> $@
	@{ printf '"' && { cat $^ | tr -d '\n' | sed 's/  */ /g' | sed 's/"/\\"/g'; } && printf '"'; } | tr -d '\n' >> $@
	@echo >> $@
	@echo "#endif" >> $@

##
# Clean project build files.
#
clean:
	$(RM) $(OBJS)
	$(RM) $(SQL_H)
	$(RM) $(TARGET)
	$(RM) $(TARGET_STATIC)
