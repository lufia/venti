#include <u.h>
#include <libc.h>

typedef struct Auto Auto;
struct Auto
{
	char	*name;
	uvlong	min;
	uvlong	max;
	uint	weight;
	uchar	alloc;
	uvlong	size;
};

#define TB (1024LL*GB)
#define GB (1024*1024*1024)
#define MB (1024*1024)
#define KB (1024)

/*
 * Order matters -- this is the layout order on disk.
 */
Auto autox[] = 
{
	{	"9fat",		10*MB,	100*MB,	10,	},
	{	"nvram",	512,	512,	1,	},
	{	"fscfg",	1024,	8192,	1,	},
	{	"fs",		200*MB,	0,	10,	},
	{	"fossil",	200*MB,	0,	4,	},
	{	"arenas",	500*MB,	0,	20,	},
	{	"isect",	25*MB,	0,	1,	},
	{	"bloom",	4*MB,	512*MB,	1,	},

	{	"other",	200*MB,	0,	4,	},
	{	"swap",		100*MB,	512*MB,	1,	},
	{	"cache",	50*MB,	1*GB,	2,	},
};

int debug;

static void
autoxpart(vlong secs, vlong secsize)
{
	int i, totw, futz;
	vlong s;

	for(;;){
		/* compute total weights */
		totw = 0;
		for(i=0; i<nelem(autox); i++){
			if(autox[i].alloc==0 || autox[i].size)
				continue;
			totw += autox[i].weight;
		}
		if(totw == 0)
			break;

		if(secs <= 0)
			sysfatal("ran out of disk space during autoxpartition.\n");

		/* assign any minimums for small disks */
		futz = 0;
		for(i=0; i<nelem(autox); i++){
			if(autox[i].alloc==0 || autox[i].size)
				continue;
			s = (secs*autox[i].weight)/totw;
			if(s < autox[i].min/secsize){
				autox[i].size = autox[i].min/secsize;
				secs -= autox[i].size;
				futz = 1;
				break;
			}
		}
		if(futz)
			continue;

		/* assign any maximums for big disks */
		futz = 0;
		for(i=0; i<nelem(autox); i++){
			if(autox[i].alloc==0 || autox[i].size)
				continue;
			s = (secs*autox[i].weight)/totw;
			if(autox[i].max && s > autox[i].max/secsize){
				autox[i].size = autox[i].max/secsize;
				secs -= autox[i].size;
				futz = 1;
				break;
			}
		}
		if(futz)
			continue;

		/* finally, assign partition sizes according to weights */
		for(i=0; i<nelem(autox); i++){
			if(autox[i].alloc==0 || autox[i].size)
				continue;
			s = (secs*autox[i].weight)/totw;
			autox[i].size = s;

			/* use entire disk even in face of rounding errors */
			secs -= autox[i].size;
			totw -= autox[i].weight;
		}
	}

	for(i=0; i<nelem(autox); i++){
		if(!autox[i].alloc)
			continue;
		if(debug)
			fprint(2, "%s %llud\n", autox[i].name, autox[i].size);
		print("fallocate -l %lld %s\n",
			secsize*autox[i].size, autox[i].name);
	}
}

vlong
xsize(char *s)
{
	char *t;
	vlong n;

	n = strtoll(s, &t, 10);
	if(t == nil)
		return n;
	switch(*t){
	case 'K':
		return n*KB;
	case 'M':
		return n*MB;
	case 'G':
		return n*GB;
	case 'T':
		return n*TB;
	default:
		return -1;
	}
}

void
usage(void)
{
	fprint(2, "usage: %s [-a partname]... [-s secsize] size[TGMK]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i;
	char *p;
	vlong size, secsize, secs;

	secsize = 0;
	ARGBEGIN {
	case 'a':
		p = EARGF(usage());
		for(i = 0; i < nelem(autox); i++){
			if(strcmp(p, autox[i].name) != 0)
				continue;
			if(autox[i].alloc){
				fprint(2, "you said -a %s more than once.\n", p);
				usage();
			}
			autox[i].alloc = 1;
		}
		break;
	case 'd':
		debug = 1;
		break;
	case 's':
		secsize = atoi(ARGF());
		break;
	default:
		usage();
	} ARGEND
	if(secsize == 0)
		secsize = 512;
	if(argc != 1)
		usage();
	size = xsize(argv[0]);
	if(size < 0)
		usage();
	secs = size / secsize;
	if(debug)
		fprint(2, "size=%lld secs=%lld sescsize=%lld\n", size, secs, secsize);
	autoxpart(secs, secsize);
}
