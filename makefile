CC=mpicc

a.out: pro2a.c
	$(CC) pro2a.c -o out1

clean:
	@rm -f *.o *.out out1
