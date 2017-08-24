project=cute
cc=gcc
source=server.c cJSON.c config.c
link=-lpthread -lm
$flags=-g
outdir=../bin
install:
	test -d $(outdir) || mkdir -p $(outdir)
	$(cc) $(flags) $(source) $(link) -o $(outdir)/$(project)
clean:
	rm -f $(outdir)/$(project)
