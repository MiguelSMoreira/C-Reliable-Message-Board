#***********************************************************************
# File Name: makefile
#	      
# Author:    Ana Catarina Sa (ist178665)e  Miguel Moreira (ist181048)
# Data:      29 de Marco de 2017 
# 
# NAME       RELIABLE MESSAGE BOARD (Projecto de RCI 2017)     
#	     Servidor de Mensagens
# 
#************************************************************************
# Compiler
CC=gcc

# Name
TARGET=msgserv

# Libraries
LIBS= -lm

# Compiler Flags
CFLAGS= -Wall -O3

# Sources
SRCFILES= msgserv.c
INCFILES= msgserv.h
OBJFILES= msgserv.o

$(TARGET): $(OBJFILES)
		$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LIBS)

msgserv.o: msgserv.c

clean::
	rm -f $(TARGET) $(OBJFILES) core.* #*

depend::
	makedepend $(SOURCES)

 
