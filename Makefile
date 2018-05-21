CC=gcc

#The D_DEBUG macro controls the debug output
#The DNOT_CMD macro makes this app work in another inconvenient way
CFLAGS=#-DNOT_CMD=1 #-Wall -pedantic -D_DEBUG
    #-ansi -pedantic -Wall -W -Wconversion -Wshadow -Wcast-qual -Wwrite-strings
ifeq ($(DEBUG),1)
CFLAGS+=-Wall -O -g
endif
ifeq ($(UDP),1)
CFLAGS+=-DUDP
endif
ifeq ($(UDP),0)
CFLAGS+=-DSYSLOG
endif

LDFLAGS=
LIBS= -lm

SOURCES=cJSON.c log_demo.c

HEADERS=$(wildcard *.h)

OBJECTS=$(SOURCES:.c=.o)

TARGET=log_demo

all: $(SOURCES) $(TARGET) $(HEADERS)

$(TARGET): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

%.o:%.cpp
	$(CC) $(CFLAGS) $(X) -c $< -o $@

clean:
	@rm -rf $(TARGET) $(OBJECTS)
	@rm -rf *~
