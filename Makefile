############## VARIABLE DEFINITIONS #################

PROJECT_NAME = bin-parser
SOURCES = bin-parser.c
EXEC = bin-parser
INCLUDES =

LINKSTATIC = no
ANSI = yes

MAKE = gmake
CC = gcc
LD = ld
AR = ar
OBJCOPY = objcopy

OBJECTS = $(SOURCES:.c=.o)
PROJROOT = $(PWD)

ifeq "$(LINKSTATIC)" "yes"
STATIC = -static
else
STATIC = 
endif

ifeq "$(ANSI)" "yes"
STANDARD = -std=c99 -pedantic
else
STANDARD = 
endif

LDFLAGS = $(STATIC) -Ttext 1000000
CFLAGS = -fno-strict-aliasing -fno-builtin -fno-omit-frame-pointer \
		 -Wall -Werror -gdwarf-2 -O1 $(STANDARD) -c

#################### RULE HEADER ####################

all: clean $(SOURCES) $(EXEC)
debug: CFLAGS += -DDEBUG -g
debug: clean $(SOURCES) $(EXEC)

################### BUILD RULES ######################

$(EXEC): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

################## GENERIC RULES #####################

%.o: %.c
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDES) -o $@ $<
	$(OBJCOPY) -R .comment -R .note $@ $@

%.a:
	rm -f $@
	$(AR) rc $@ $^

################### TEST RULES ######################

.PHONY: test
test:
	make all \
&& echo "Test1" && ./$(EXEC) test1.bin test1.tout && diff test1.out test1.tout \
&& echo "Test2" && ./$(EXEC) test2.bin test2.tout && diff test2.out test2.tout \
&& echo "Test3" && ./$(EXEC) test3.bin test3.tout && diff test3.out test3.tout \
&& echo "Test4" && ./$(EXEC) test4.bin test4.tout && diff test4.out test4.tout \
&& echo "Test5" && ./$(EXEC) test5.bin test5.tout \
|| echo "Test6" && ./$(EXEC) test6.bin test6.tout && diff test6.out test6.tout \
&& echo "Test7" && ./$(EXEC) test7.bin test7.tout && diff test7.out test7.tout \
&& echo "Test8" && ./$(EXEC) test8.bin test8.tout && diff test8.out test8.tout \
&& echo "Test9" && ./$(EXEC) test9.bin test9.tout && diff test9.out test9.tout

################## PACKAGE RULES ####################

.PHONY: tar
tar: clean
	tar -cf $(PROJECT_NAME).tar * --exclude=*.{tar}

################# CLEANING RULES #####################

.PHONY: clean

clean:
	rm -rf *.o *.tout *.tar $(EXEC)

%clean:
	$(error "Unknown cleaning target")