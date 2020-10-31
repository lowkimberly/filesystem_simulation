/*

  simulation of a file system
 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int verbose;

struct block {
  char* type;
  char* first_two_bytes;
  char* name;
  char* longname;

  short mfr_bytes_in_file;
  char* mfr_data;

  short dir_numfiles;
  char* dir_name[6];
  short dir_blockno[6];
  
  char* data;

  int free_list[256];
};

int error_checking(int argc, char* argv[]) {
  if (argc != 3)  {
    printf("Wrong number of arguments.\n");
    return 1;
  }
  FILE* command_file=fopen(argv[1],"r");
  if(command_file == NULL)  {
    printf("Can't open the file.\n");
    return 1;
  }
  if (strcmp(argv[2],"-S") != 0 && strcmp(argv[2],"-V") != 0) {
    printf("Don't recognize the flag.\n");
    return 1;
  }
  return 0;
}

int disk_setup(struct block* disk){
  int i;
  for (i=0; i< 256; i++){
    disk[i].type = malloc(5*sizeof(char));
    disk[i].first_two_bytes = malloc(3*sizeof(char));
    disk[i].name=malloc(8*sizeof(char));
    disk[i].longname=malloc(256*sizeof(char));
    disk[i].mfr_bytes_in_file = -1;
    disk[i].mfr_data = malloc(128*sizeof(char));
    disk[i].dir_numfiles = -1;
    int j;
    for(j=0;j<6;j++){
      disk[i].dir_name[j]=malloc(8*sizeof(char));
      disk[i].dir_blockno[j]=-1;
    }
    disk[i].data=malloc(65*sizeof(char));
  }

  strcpy(disk[0].type,"free");
  int free_index;
  for(free_index=0; free_index<256; free_index++) {
    (disk[0].free_list)[free_index]=0;
  }
  (disk[0].free_list)[0]=1;

  strcpy(disk[1].type,"dir");
  strcpy(disk[1].first_two_bytes,"DL");
  disk[1].dir_numfiles=1;
  strcpy(disk[1].dir_name[0],"..");
  disk[1].dir_blockno[0]=1;

  (disk[0].free_list)[1]=1;
  

  return 0;
}

int next_free_index(int free_list[]) {
  int f;
  for (f=0;f<256;f++) {
    if (free_list[f]==0) {
      return f;
    }
  }
  return -1;
}


int my_export(struct block* disk, char* direct, char* command) {
  //export filename externalfile
  char* filename=malloc(256*sizeof(char));
  char* externalfile=malloc(256*sizeof(char));
  char* tok;
  tok = strtok (command," ");
  tok = strtok (NULL," ");
  strcpy(filename,tok);
  tok = strtok (NULL," ");
  strcpy(externalfile,tok);

  int wanted_index;
  for (wanted_index = 1; wanted_index < 50; wanted_index++){
    if (strcmp(disk[wanted_index].name,filename)==0) {
      break;
    }
  }

  FILE* new_extern=fopen(externalfile,"w");

  if (disk[wanted_index].mfr_bytes_in_file < 61){
    fprintf(new_extern,"%s",disk[wanted_index].mfr_data);
  }
  else {
    //if it's big, open mfr_data, read start_block, number of blocks, over
    int startblock=0;
    int numblocks=0;
    tok = strtok(disk[wanted_index].mfr_data," ");

    if (tok != NULL) {
      startblock=atoi(tok);
    }
    tok=strtok(NULL," ");
    if (tok != NULL) {
      numblocks=atoi(tok);
    }

    while (tok != NULL){
      break;
    }
  }  
  return 0;
}


int my_import(struct block* disk, char* direct, char* command) {
  int f = next_free_index(disk[0].free_list);
  if (f==-1) {
    printf("No room to copy file.\n");
    return 1;
  }

  //Get the two file names
  char* filename=malloc(256*sizeof(char));
  char* externalfile=malloc(256*sizeof(char));
  char* tok;
  tok = strtok (command," ");
  tok = strtok (NULL," ");
  strcpy(filename,tok);
  tok = strtok (NULL," ");  
  strcpy(externalfile,tok);

  //make the file names correct
  char* longintern=malloc(1024*sizeof(char));
  char* first=malloc(1024*sizeof(char));
  char* second=malloc(1024*sizeof(char));

  tok=strtok(filename,"/");
  strcpy(first,tok);

  tok=strtok(NULL,"/");

  if (tok != NULL) {
    strcpy(second,tok);
  }

  if (filename[0]=='/') {
    strcpy(longintern,"/");
  }
  else {
    strcpy(longintern,direct);
  }

  if (strlen(second) == 0) {
    strcat(longintern,first);
    strcpy(disk[f].name,first);
  }
  else {
    strcat(longintern,first);
    strcat(longintern,"/");
    strcat(longintern,second);

    strcpy(disk[f].name,second);
  }
  strcpy(disk[f].longname,longintern);

  struct stat* ptr_stats = malloc(sizeof(struct stat));
  stat(externalfile,ptr_stats);
  int filesize = ptr_stats->st_size;

  char* databuf=malloc(1024*sizeof(char));
  strcpy(disk[f].type, "mfr");
  disk[f].mfr_bytes_in_file=filesize;
  FILE* ext_stream = fopen(externalfile,"r");

  //update directory, or the root
  if (strlen(second) >  0){
    int b;
    for (b=0;b<256;b++) {
      if (strcmp(disk[b].name,first)==0){
	strcpy(disk[b].dir_name[(disk[b].dir_numfiles)],second);
	disk[b].dir_blockno[(disk[b].dir_numfiles)]=f;
	disk[b].dir_numfiles++;
	break;
      }
    }
  }
  else {
    strcpy(disk[1].dir_name[(disk[1].dir_numfiles)],first);
    disk[1].dir_blockno[(disk[1].dir_numfiles)]=f;
    disk[1].dir_numfiles++;
  }


  if (filesize < 61)  {
    strcpy(disk[f].first_two_bytes,("RL"));
    fgets(databuf,60,ext_stream);
    strcpy(disk[f].mfr_data,databuf);
    disk[0].free_list[f]=1;
  }
  else {
    strcpy(disk[f].first_two_bytes,("RB"));

    //the mfr for this thing is now occupied and has the start block
    disk[0].free_list[f]=1;

    int freeblock=next_free_index(disk[0].free_list);
    char* intbuf=malloc(8*sizeof(char));
    //starting block
    sprintf(intbuf," %d ",freeblock);
    strcat(disk[f].mfr_data,intbuf);

    //so far we have added 0 lines.
    int linescounter=0;

    while (fgets(databuf,64,ext_stream) != NULL){
      freeblock=next_free_index(disk[0].free_list);      
      strcpy(disk[freeblock].type,"data");
      strcpy(disk[freeblock].data,databuf);

      //starting block + lines from block less than the actual index of block
      if (atoi(intbuf)+linescounter < freeblock){
	//print lines passed, then print the start of the next group
	sprintf(intbuf,"%d ",linescounter);
	strcat(disk[f].mfr_data,intbuf);
	
	//intbuf = new starting block, reset lines, 
	sprintf(intbuf,"%d ",next_free_index(disk[0].free_list));
	strcat(disk[f].mfr_data,intbuf);
	linescounter=0;
      }
      linescounter++;
      disk[0].free_list[freeblock]=1;
    }

    sprintf(intbuf,"%d ",linescounter);
    strcat(disk[f].mfr_data,intbuf);
  }
  if (verbose == 0) { printf("Putting MFR for %s in block %d\n",filename,f); }
  return 0;
}

int my_copy(){
  return 0;
}

int my_rename() {
  return 0;
}

int my_mkdir(struct block* disk, char* direct, char* command) {
  int f=next_free_index(disk[0].free_list);
  if (f==-1) {
    printf("No room to copy file.\n");
    return 1;
  }

  strcpy(disk[f].type,"dir");
  strcpy(disk[f].first_two_bytes,"DL");
  disk[f].dir_numfiles=1;
  strcpy(disk[f].dir_name[0],"..");
  disk[f].dir_blockno[0]=1;
  (disk[0].free_list)[f]=1;

  char* tok;
  char* newdir=malloc(1024*sizeof(char));
  tok = strtok (command," ");
  tok = strtok (NULL," ");
  strcpy(newdir,direct);
  strcat(newdir,tok);
  strcat(newdir,"/");

  strcpy(disk[f].name,tok);
  strcpy(disk[f].longname,newdir);

  if (verbose == 0) { printf("Putting directory %s in block %d\n",disk[f].name,f); }
  

  return 0;
}

int my_delete() {
  return 0;
}

int my_chdir(char* direct, char* command) {
  char* tok=malloc(1024*sizeof(char));
  tok = strtok (command," ");
  tok = strtok (NULL," ");
  strcat(direct,"/");
  strcat(direct,tok);
  strcat(direct,"/");
  return 0;
}

int my_append(){
  return 0;
}

int my_rmdir() {
  return 0;
}

int my_quit(struct block disk[]) {
  FILE* dump_stream = fopen("dump.txt","w");

  //start at 1 so I don't dump the freelist too
  int block_index;
  for(block_index=1; block_index < 256; block_index++)  {
    char* out_str=malloc(1024*sizeof(char));
    //mfr
    if(strncmp(disk[block_index].type, "mfr", 3)==0){
      strcpy(out_str,disk[block_index].first_two_bytes);
      strcat(out_str," ");
      char* intbuf=malloc(8*sizeof(char));
      sprintf(intbuf,"%d",disk[block_index].mfr_bytes_in_file);
      strcat(out_str, intbuf);
      strcat(out_str," ");
      strcat(out_str,disk[block_index].mfr_data);
    }
    //is a dir.
    else if (strncmp(disk[block_index].type,"dir",3)==0){
      strcpy(out_str,disk[block_index].first_two_bytes);
      strcat(out_str," ");

      char* intbuf=malloc(8*sizeof(char));
      sprintf(intbuf,"%d",disk[block_index].dir_numfiles);
      strcat(out_str, intbuf);

      int i;
      for (i = 0; disk[block_index].dir_blockno[i] != -1; i++){
	strcat(out_str," ");
	strcat(out_str,disk[block_index].dir_name[i]);
	strcat(out_str," ");
	sprintf(intbuf,"%d",disk[block_index].dir_blockno[i]);
	strcat(out_str,intbuf);	
      }
    }
    //if it's not either, it's data
    else if (disk[block_index].type != NULL) {
      strncpy(out_str,disk[block_index].data,64);
    }
    fprintf(dump_stream,"%d %s\n",block_index,out_str);
  }
  return 0;
}


int main(int argc, char* argv[]) {
  if (error_checking(argc,argv) == 1) { return 1; }

  if (strcmp(argv[2],"-V") == 0) { verbose = 0; }
  else {verbose = 1;}

  struct block* disk=malloc(256*sizeof(struct block));
  disk_setup(disk);

  char* direct=malloc(1024*sizeof(char));
  strcpy(direct,"./");

  char* command_buf = malloc(1024*sizeof(char));
  FILE* command_file = fopen(argv[1],"r");
  while(fgets(command_buf,1024,command_file) != NULL) {
    char* cleanup=malloc(1024*sizeof(char));
    strncpy(cleanup,command_buf,strlen(command_buf)-1);
    strcpy(command_buf,cleanup);
    if (strncmp(command_buf,"export",6)==0)      { my_export(disk,direct,command_buf); }
    else if (strncmp(command_buf,"import",6)==0) { my_import(disk,direct,command_buf);  }
    else if (strncmp(command_buf,"copy",4)==0)   { my_copy(); }
    else if (strncmp(command_buf,"rename",6)==0) { my_rename(); }
    else if (strncmp(command_buf,"mkdir",5)==0)  { my_mkdir(disk,direct,command_buf);    }
    else if (strncmp(command_buf,"delete",6)==0) { my_delete();    }
    else if (strncmp(command_buf,"chdir",5)==0)  { my_chdir(direct,command_buf);    }
    else if (strncmp(command_buf,"append",6)==0) { my_append();    }
    else if (strncmp(command_buf,"rmdir",5)==0)  { my_rmdir();  }
    else if (strncmp(command_buf,"quit",4)==0)   { my_quit(disk); break; }
  }
  return 0;
}
