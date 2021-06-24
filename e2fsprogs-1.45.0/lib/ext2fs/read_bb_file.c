/*
 * read_bb_file.c --- read a list of bad blocks from a FILE *
 *
 * Copyright (C) 1994, 1995, 2000 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Library
 * General Public License, version 2.
 * %End-Header%
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <time.h>
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "ext2_fs.h"
#include "ext2fs.h"

/*
 * Reads a list of bad blocks from  a FILE *
 */

#define BILLION 1000000000L

unsigned long long global_read_bad_time = 0;
unsigned long long global_read_bad_count = 0;
unsigned long long global_total_while_time = 0;
unsigned long long global_total_while_count = 0;
unsigned long long global_fgets_time = 0;
unsigned long long global_fgets_count = 0;
unsigned long long global_sscanf_time = 0;
unsigned long long global_sscanf_count = 0;



unsigned long long calclock_Moco(struct timespec *myclock, unsigned long long *total_time, unsigned long long *total_count){
        unsigned long long timedelay=0,temp=0, temp_n=0;
        if(myclock[1].tv_nsec >= myclock[0].tv_nsec){
                temp = myclock[1].tv_sec - myclock[0].tv_sec;
                temp_n = myclock[1].tv_nsec - myclock[0].tv_nsec;
                timedelay = BILLION * temp + temp_n;
        }
        else {
                temp = myclock[1].tv_sec - myclock[0].tv_sec -1;
                temp_n = BILLION + myclock[1].tv_nsec - myclock[0].tv_nsec;
                timedelay = BILLION * temp + temp_n;
        }

        __sync_fetch_and_add(total_time,timedelay);
        __sync_fetch_and_add(total_count, 1);

        return timedelay;
}



errcode_t ext2fs_read_bb_FILE2(ext2_filsys fs, FILE *f,
			       ext2_badblocks_list *bb_list,
			       void *priv_data,
			       void (*invalid)(ext2_filsys fs,
					       blk_t blk,
					       char *badstr,
					       void *priv_data))
{
	errcode_t	retval;
	blk64_t		blockno;
	int		count;
	char		buf[128];
	struct timespec local_time[2];
	char * rev = NULL;

	struct timespec total_while_time[2];
	struct timespec fgets_time[2];
	struct timespec sscanf_time[2];
	struct timespec read_bad_time[2];
	unsigned long long while_count = 0;
	if (fs)
		EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!*bb_list) {
		retval = ext2fs_badblocks_list_create(bb_list, 10);
		if (retval)
			return retval;
	}
//Moco finde -c path
printf("HIIIIII\n");

	clock_gettime(CLOCK_MONOTONIC, &total_while_time[0]);
	while (!feof (f)) {
		while_count++;

//		printf("???!?!? %lld \n",sizeof(buf));

		clock_gettime(CLOCK_MONOTONIC, &fgets_time[0]);
		if (fgets(buf, sizeof(buf), f) == NULL){
			clock_gettime(CLOCK_MONOTONIC, &fgets_time[1]);
			calclock_Moco(fgets_time, &global_fgets_time, &global_fgets_count);
			printf("%s, fgets break\n", __func__);
			break;
		}
		clock_gettime(CLOCK_MONOTONIC, &fgets_time[1]);
		calclock_Moco(fgets_time, &global_fgets_time, &global_fgets_count);
		//sysganda
/*
		clock_gettime(CLOCK_MONOTONIC, &fgets_time[0]);
		rev = fgets(buf, sizeof(buf), f);
		clock_gettime(CLOCK_MONOTONIC, &fgets_time[1]);
		calclock_Moco(fgets_time, &global_fgets_time, &global_fgets_count);
		if(rev == NULL)	
			break;
*/

//		printf("why??\n");
		clock_gettime(CLOCK_MONOTONIC, &sscanf_time[0]);
		count = sscanf(buf, "%llu", &blockno);
		clock_gettime(CLOCK_MONOTONIC, &sscanf_time[1]);
		calclock_Moco(sscanf_time, &global_sscanf_time, &global_sscanf_count);

		if (count <= 0){
			printf("%s, count <= 0 ???\n", __func__);
			continue;
		}

		/* Badblocks isn't going to be updated for 64bit */
		if (blockno >> 32){
			clock_gettime(CLOCK_MONOTONIC, &total_while_time[1]);
			calclock_Moco(total_while_time, &global_total_while_time, &global_total_while_count);
			return EOVERFLOW;
		}
		if (fs &&
		    ((blockno < fs->super->s_first_data_block) ||
		     (blockno >= ext2fs_blocks_count(fs->super)))) {
			if (invalid)
				(invalid)(fs, blockno, buf, priv_data);
			printf("%s, continue???\n", __func__);
			continue;
		}


		clock_gettime(CLOCK_MONOTONIC, &read_bad_time[0]);
		retval = ext2fs_badblocks_list_add(*bb_list, blockno);
		clock_gettime(CLOCK_MONOTONIC,&read_bad_time[1]);
		calclock_Moco(read_bad_time, &global_read_bad_time, &global_read_bad_count);

		if (retval){
			printf("%s, retval here?\n", __func__);
			return retval;
		}
	}

clock_gettime(CLOCK_MONOTONIC, &total_while_time[1]);
calclock_Moco(total_while_time, &global_total_while_time, &global_total_while_count);

printf("while called : %llu \n",while_count);
printf("BYEEEEE\n");

printf("MOCO ext2fs_badblocks_list time :	time : %llu , count : %llu\n	MOCO fgets time : 	time : %llu , count : %llu\n  	MOCO sscanf time : 	time : %llu , count : %llu\n 	MOCO total time : time : %llu , count : %llu \n ",global_read_bad_time,global_read_bad_count,global_fgets_time, global_fgets_count,global_sscanf_time, global_sscanf_count, global_total_while_time, global_total_while_time,global_total_while_count  );
	return 0;
}

struct compat_struct {
	void (*invalid)(ext2_filsys, blk_t);
};

static void call_compat_invalid(ext2_filsys fs, blk_t blk,
				char *badstr EXT2FS_ATTR((unused)),
				void *priv_data)
{
	struct compat_struct *st;

	st = (struct compat_struct *) priv_data;
	if (st->invalid)
		(st->invalid)(fs, blk);
}


/*
 * Reads a list of bad blocks from  a FILE *
 */
errcode_t ext2fs_read_bb_FILE(ext2_filsys fs, FILE *f,
			      ext2_badblocks_list *bb_list,
			      void (*invalid)(ext2_filsys fs, blk_t blk))
{
	struct compat_struct st;

	st.invalid = invalid;

	return ext2fs_read_bb_FILE2(fs, f, bb_list, &st,
				    call_compat_invalid);
}


