# Variables 
INCLUDES =  
SOURCES = server.c   
OBJFILES = server.o   
EXEC = prog.bin 

# Compilation parameters 
CP = -Wall -Wextra -pthread  # Added -pthread for thread support

# Rules for suffixes 
.SUFFIXES: .c .o  

# Transform .c into .o ; $< -- filename 
.c.o:  
	gcc -g ${CP} -c $<    

# Main program 
${EXEC}: ${OBJFILES}  
	gcc -g ${CP} -o ${EXEC} ${OBJFILES}  

# Run the program 
run: ${EXEC}  
	./${EXEC}  

# Clean up object files and executable 
clean:
	rm -f ${OBJFILES} ${EXEC}
