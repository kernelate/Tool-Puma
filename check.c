#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <stdio.h>
#include <stdlib.h>
#define MAX_BUFF    255

float getrequiredsize(const char *filename)
{
    struct stat st;
    float size;
    if (stat(filename, &st) == 0)
    {
    	if(strncmp(filename,"/backup/data.tgz",16)==0) // if not update file
    		size = ((st.st_size + 200000000) / 1000000); //backupfile + 200MB
    	else
    		size = (st.st_size / 1000000) *2.1;    		//210% of update file
    	return size;
    }
    return -1;
}

double getrequiredsizefrombytes(double filesize)
{
	float size;
	size = (filesize / 1000000) *2.1;
	return size;
} 

int checker(char* filepath, char* checksum)
{

    FILE *file;
    char command[255];
    char hash[100];

    char *f_check = "/cache/file_check";
    sprintf(command, "busybox md5sum %s > %s", filepath, f_check);
    system(command);
    puts(command);
    
    if((file = fopen(f_check, "r")) == NULL){
        printf("file does not exist\n");
    }
    else{
        fscanf(file, "%s", hash);
    }

//    printf("\n\nhash %s\nchec %s\n", hash, temp);
    if(strcmp(hash, checksum)){
    //    printf("not the same checksum\n");
        return -1;
    }
    
//    printf("the same file continue\n");   
    return 0;
 //   fclose(file);
} 

int check_disk_space(char *path, double required_size)
{
	struct statfs st;
	float free_space;

	if((statfs(path, &st)) < 0 ) {
		printf("Failed to stat %s:\n", path);
		return -1;
	} else {
		free_space = (st.f_bsize * st.f_bfree)/(1024*1024); //MB
//		printf("\tBlock size: %u\n", st.f_bsize);
//		printf("\tTotal no blocks: %i\n", st.f_blocks);
//		printf("\tFree space: %fMB\n", free_space);

		if(required_size < free_space)
			return 0;
		else
		{
			printf("\tupdate system requires free space at least %fMB\n", required_size);
			return -1;
		}
	}
}

int f_size(char *file) {
	struct stat f;
	if (stat(file, &f) < 0) {
		fprintf(stderr, "%s: \n", file);
		return -1;
	} else {

		int size = f.st_size;

		return size;
	}

}
