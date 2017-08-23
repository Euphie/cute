project=cute
cc=gcc
source=server.c cJSON.c config.c
link=-lpthread -lm
outdir=bin
target:
	$(cc) $(source) $(link) -o $(outdir)/$(project)
clean:
	rm -f $(outdir)/$(project)
