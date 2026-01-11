CC = g++
OPT = -O3
OPT = -g
WARN = -Wall
ERR = -Werror

CFLAGS = $(OPT) $(WARN) $(ERR) $(INC) $(LIB)

SIM_SRC = main.cc cache.cc bus_ops.cc

SIM_OBJ = main.o cache.o bus_ops.o

all: smp_cache
	@echo "smp_cache build complete."

smp_cache: $(SIM_OBJ)
	$(CC) -o smp_cache $(CFLAGS) $(SIM_OBJ) -lm
	@echo "----------------------------------------------------------"
	@echo "-----------CACHE COHERENCE SIMULATOR(SMP_CACHE)-----------"
	@echo "----------------------------------------------------------"
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc

clean:
	rm -f *.o smp_cache

clobber:
	rm -f *.o