project=cute
cc=gcc
source=server.c cJSON.c config.c
link=-lpthread -lm
flags=-g
outdir=../bin
confdir=../config
cmddir=../cmd
target:
	test -d $(outdir) || mkdir -p $(outdir)
	$(cc) $(flags) $(source) $(link) -o $(outdir)/$(project)
install:
	cp -a $(outdir)/$(project) /bin/$(project)
	cp -a $(confdir)/$(project).ini /etc/$(project).ini
	cp -a $(cmddir)/$(project)d.sh /etc/init.d/$(project)d
	chmod +x /etc/init.d/$(project)d
	chkconfig --add $(project)d
uninstall:
    service $(project)d stop
	chkconfig --del $(project)d
	rm -f /etc/init.d/$(project)d
	rm -f /etc/$(project).ini
	rm -f /bin/$(project)
	rm -f $(outdir)/$(project)

