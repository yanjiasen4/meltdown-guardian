#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "msrop.h"

int msr_debug;

static int *fd;

void
msr_init(){
	int i;
	char filename[1025];
	static int initialized = 0;
	if( initialized ){
#ifdef _DEBUG
		fprintf(stderr, "%s:%d: already initialized\n", __FILE__, __LINE__);
#endif
		return;
	}
	fd = (int*)calloc(CPU_CORES, sizeof(int));
	for (i = 0; i < CPU_CORES; i++){
	  /*! @todo check this msr selection; it may be incorrect on some
	    machines, e.g. non-fio hyperion nodes
	   */
		snprintf(filename, 1024, "/dev/cpu/%d/msr", i);
		fd[i] = open( filename, O_RDWR );
		if(fd[i] == -1){
			char errMsg[1024];
			snprintf(errMsg, 1024, "%s::%d  Error opening %s\n", __FILE__, __LINE__, filename);
			errMsg[1023] = 0;
			perror(errMsg);
		}
	}
	initialized = 1;
}

void
msr_finalize(){
	int i, rc;
	char filename[1025];
	for( i = 0; i < CPU_CORES; i++){
		if(fd[i]){
			rc = close(fd[i]);
			if( rc != 0 ){
				snprintf(filename, 1024, "%s::%d  Error closing file /dev/cpu/%d/msr\n",
						__FILE__, __LINE__, i);
				perror(filename);
			}
		}
	}
}

void
msr_write(int core, off_t msr, uint64_t val) {
    int rc;
    char error_msg[1025];

	if(msr_debug){
	  fprintf(stderr, "%s::%d write msr=0x%lx val=0x%lx\n",
		  __FILE__, __LINE__, msr, val);
        }
	rc = pwrite( fd[core], &val, (size_t)sizeof(uint64_t), msr );
	if( rc != sizeof(uint64_t) ){
		snprintf( error_msg, 1024, "%s::%d  pwrite returned %d.  fd[%d]=%d, core=%d, msr=%ld (%lx), val=%ld (0x%lx).\n",
				__FILE__, __LINE__, rc, core, fd[core], core, msr, msr, val, val);
		perror(error_msg);
	}
}

void
msr_read(int core, off_t msr, uint64_t *val){
	int rc;
	char error_msg[1025];
	rc = pread( fd[core], (void*)val, (size_t)sizeof(uint64_t), msr );
	if( rc != sizeof(uint64_t) ){
		snprintf( error_msg, 1024, "%s::%d  pread returned %d.  core=%d, msr=%ld (0x%lx), val=%ld (0x%lx).\n",
				__FILE__, __LINE__, rc, core, msr, msr, *val, *val );
		perror(error_msg);
	}
	if( msr_debug ){
		fprintf(stderr, "%s::%d c%d read msr=0x%lx val=0x%lx\n",
						__FILE__, __LINE__, core, msr, *val);
	}
}
