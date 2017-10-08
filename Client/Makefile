#***********************************************************************
# File Name: makefile
#	      
# Author:    Ana Catarina Sa (ist178665)e  Miguel Moreira (ist181048)
# Data:      29 de Marco de 2017
# 
# NAME       RELIABLE MESSAGE BOARD (Projecto de RCI 2017) 
#	     Aplicação Cliente    
# 
#************************************************************************
# Compiler
CC=gcc

# Name
TARGET=rmb

# Libraries
LIBS= -lm

# Compiler Flags
CFLAGS= -Wall -O3

# Sources
SRCFILES= rmb.c
INCFILES= rmb.h
OBJFILES= rmb.o

$(TARGET): $(OBJFILES)
		$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LIBS)

rmb.o: rmb.c

clean::
	rm -f $(TARGET) $(OBJFILES) core.* #*

depend::
	makedepend $(SOURCES)

 
