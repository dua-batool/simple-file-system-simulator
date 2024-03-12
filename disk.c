#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FILENAME_MAXLEN 8  
#define TOTAL_INODES 16
#define TOTAL_BLOCKPTRS 8 

// inode
typedef struct inode {
  int  dir;  // boolean value. 1 if it's a directory.
  char name[FILENAME_MAXLEN];
  int  size;  // actual file/directory size in bytes.
  int  blockptrs [TOTAL_BLOCKPTRS];  // direct pointers to blocks containing file's content.
  int  used;  // boolean value. 1 if the entry is in use.
  int  rsvd;  // reserved for future use
} inode;

// superblock
typedef struct superblock {
  char freeblocklist[128];       // assigning 128 bytes to the free block list in the superblock
  inode inodes[TOTAL_INODES];    // for the 16 inodes in the superblock
} superblock;

// directory entry
typedef struct dirent {
  char name[FILENAME_MAXLEN];
  int  namelen;  // length of entry name
  int  inode;  // this entry inode index
} dirent;

// datablock
typedef struct datablock {
  char index[1024];          // each index of datablock consists of 1024 bytes
} datablock;

// helper functions
// checking if the directory already exists
int exists (char * directory, superblock * s) {
  for (int i = 0; i < TOTAL_INODES; i++){                   // looping over each inode
    if (strcmp((*s).inodes[i].name , directory) == 0) {     // checking if the name of the directory matches
      return i;                                             // returning the inode number if the directory is found
    }
  }
  return -1;        // if the directory does not exist in the inode table
}

// checking if all inodes are in use or not
int check_space (superblock * s){
  int left = -1;                                 // initially keeping it -1, it would change to 1 if there is space
  for(int i = 0; i < TOTAL_INODES; i++){         // looping over each inode 
    if ((*s).inodes[i].used == 0) {              // checking if that inode is in use
      left = 1;                                  // if not in use then returning 1 to indicate that there is space left
      break;
    }
  }
  return left;
}

// setting up the initial values for superblock and datablock
void set_null (superblock * s, datablock * d_block[]){
  for (int i = 0; i < TOTAL_INODES; i++) {
    s->inodes[i].used = 0;                      // to indicate that inode is not in use
    s->inodes[i].rsvd = 0;                      // to indicate that inode is not reserved
    strcpy(s->inodes[i].name, "");              // initializing the name with empty string
    s->inodes[i].dir = 0;                       // initially considering that it is not a directory, would change to 1 in case of a directory
    s->inodes[i].size = 0;                      // initializing size with 0
    for (int j = 0; j < TOTAL_BLOCKPTRS; j++){       // looping over each blockpointer of each inode
      s->inodes[i].blockptrs[j] = -1;                // to indicate that the blockpointer is not pointing towards any index of the data block
    }
  }

  s->freeblocklist[0] = '1';              // the first element of freeblock list is occupied since it is the superblock
  for (int i = 1; i < 128; i++){          // looping over the entire freeblocklist except for its first index
    s->freeblocklist[i] = '0';            // to indicate that index of datablock are initially not in use
    for (int j = 0; j < 1024; j++) {
      d_block[i-1]->index[j] = '\0';      // setting each index of datablock to \0
    }                                     // i-1 because the indexes 1-127 of freeblocklist represent the indexes 0-126 of the datablock
  }
}

//checking for index with no data in data blocks
int data_index (superblock * s){
  for (int i = 1; i < 128; i++){            // i starting from 1 since d_block[0] contains super block
    if (s->freeblocklist[i] == '0') {       // finding the index of freeblocklist which is not used
      return i-1;                           // returning index of datablock
    }
  }
  return -1;                                // in case all indexes of freeblocklist are in use
}

//checking for index with no data in block pointers of inodes
int block_index (inode j){
  for (int i = 0; i < TOTAL_BLOCKPTRS; i++){       // looping over all blockpointers of the given inode
    if (j.blockptrs[i] == -1) {                    // checking if they are not in use
      return i;                                    // returning the index of the blockpointer which is not in use
    }
  }
  return -2;                                       // in case all the indexes of the blockpointers are in use
}

//functions
// create file
void createfile (char * filename, char * filesize, superblock * s, datablock * d_block[]) {
  // creating a path array that contains each directory or file from the abosulute path 
  char *path[TOTAL_BLOCKPTRS];                // 8 is the longest path there could be since there are only 8 blockpointers for each inode
  char *single = strtok(filename, "/");       // separating the directories or files with '/'
  int i = 0;

  while (single != NULL) {
    path[i++] = single;              // adding all directories or files in the path
    single = strtok(NULL, "/");      
  }

  if(strlen(path[i-1]) > FILENAME_MAXLEN) {                       // path[i-1] is the name of the file to be created
    printf("Name of the file cannot exceed 8 characters.\n");     // exiting if length of the name exceeds 8 characters
    exit(1);
  }

  if(exists(path[i-1],s) != -1) {
    printf("The file already exists.\n");
    exit(1);                                       // exiting if the file already exists in the inode table
  }

  int value = i;
  int size = atoi(filesize);          // converting the size to integer
  char * block[size];                 // making block with length filesize
  for (int m = 0; m < size; m++) {
    block[m] = '\0';                  // initializing each index of the block with '\0'
  }

  char *alphabets[26];               // making an array pointing to characters a-z
  for (int u = 0; u < 26; u++) {
    char letter[2];
    letter[0] = 'a' + u;            // to get the next alphabet
    letter[1] = '\0';               // null-terminating the string
    alphabets[u] = (char *)malloc(2 * sizeof(char));     // allocating alphabets on heap         
    if (alphabets[u] == NULL) {
      perror("malloc");                                  // in case of memory allocation failure
      exit(1);
    }
    strcpy(alphabets[u], letter);    // putting the alphabet into the alphabet array
  }

  int previous = -1;    // variable used to keep track of the inode of the previous directory

  for (int j = 0; j < i; j++) {                                       // looping over each index of path
    printf("Element in create file: %d: %s\n", j, path[j]);           // to print the current element of the path
    printf("Parent value in create file: %d \n", exists(path[j],s));     // checking if this already exists in the inode table
    int parent = -1;                  // initially considering that it does not exist
    parent = exists(path[j],s);       // parent will contain the inode number if it does exist

    for (int k = 0; k < TOTAL_INODES; k++){       // looping over each inode
      if ((*s).inodes[k].used == 0) {             // k will be the inode number which is not used 

        if (previous != -1 && parent == -1){       // if the previous element is the parent of the current element and the current element does not already exist
          if (path[j] != path[i-1]) {
            printf("The directory %s at the given path does not exist.\n", path[j]);    // error checking for wrong path
            exit(1);
          }
          int d_index = data_index(s);                                      // finding the index of the datablock which is not in use
          printf ("this is d_index in create file: %d \n", d_index);
          int b_index = block_index((*s).inodes[previous]);                 // finding the index of the blockpointer of the inode which is not in use
          printf("Name of previous here: %d \n", previous);
          printf ("this is b_index in create file: %d \n", b_index);
          if (d_index != -1 && b_index != -2) {                         // if there is space both in the datablock and the blockpointers
            char inode_num[20];                                         // converting the inode number to char to put it into the datablock
            sprintf(inode_num, "%d", k);
            inode_num[strlen(inode_num)] = '\0';                        // null-terminating 
            printf("this is converted to char: %s \n", inode_num);
            strcpy(d_block[d_index]->index , inode_num);                // putting the inode number into the index of the datablock
            (*s).freeblocklist[d_index + 1] = '1';                      // changing the status of the freeblocklist
            (*s).inodes[previous].blockptrs[b_index] =  d_index;        // the parent of the current directory is pointing towards the datablock index that has inode number of current directory
            printf("%d",(*s).inodes[previous].blockptrs[b_index]);
            printf(" is the value stored in the blockpointer %d of inode %d in create file\n", b_index, previous);
          } else if(d_index == -1) {
            printf ("No Space left in the data block.\n");      // exiting if no space in the datablock
            exit(1);
          } else if(b_index == -2) {
            printf("All 8 blockpointers of the file is pointing towards some data indes.\n");        // exiting if no space in the blockpointer
            exit(1);
          }
        }

        // changing the attributes if the inode does not already exist
        if (parent == -1) {
          (*s).inodes[k].used = 1;
          (*s).inodes[k].dir = 0;
          strcpy((*s).inodes[k].name, path[j]);
          (*s).inodes[k].size = atoi(filesize);
          (*s).inodes[k].rsvd = 0;

          // changing the size of the directories in the path in accordance with the size of the file
          int the_inode = k;                                 // current inode
          for (int index = 0; index < value; index++){       // looping through all the previous elements of the current inode in the path
            int escape = 0;                                  // variable to keep track of when to exit the outer loop
            int data_index;                                 
            for (int g = 0; g < TOTAL_INODES; g++){
              for (int h = 0; h < TOTAL_BLOCKPTRS; h++){
                data_index = (*s).inodes[g].blockptrs[h];               // finding the index of the data block of each of the block pointers of each inode     
                if (atoi(d_block[data_index]->index) == the_inode){     // if the inode in that datablock matches the current inode then it means we found its parent
                  (*s).inodes[g].size = (*s).inodes[g].size + atoi(filesize);      // changing the size of the parent
                  printf("this is the size of %s: %d \n", (*s).inodes[g].name, (*s).inodes[g].size);
                  the_inode = g;           // current inode becomes the parent inode so that we can change size recursively
                  escape = 1;              // changing its value to indicate that we need to exit the outerloop
                  break;
                }
              }
              if (escape == 1){
                break;                // exiting the outerloop
              }
            }
          }

          // filling the block array with alphabets
          for (int i = 0; i < size; i++) {
            block[i] = (char *)malloc(strlen(alphabets[i % 26]) + 1);     // allocating block on heap
            strcpy(block[i], alphabets[i % 26]);                          // copying the string
          }

          int f_index = data_index(s);       // finding the index to store alphabets in the datablock
          printf ("this is the free index for storing alphabets in create file: %d \n", f_index);
          if (f_index == -1){
            printf("No space left in the data block.\n");
            exit(1);                                          // exiting if there is no space to add alphabets in the datablock
          }

          for (int b = 0; b < size; b++) {
            d_block[f_index]->index[b] = *block[b];     // adding block to the datablock
            free(block[b]);                             // free the memory on heap for block
          }
          for (int v = 0; v < 26; v++) {
            free(alphabets[v]);                         // free the memory on heap for alphabets
          }

          int fblock_index = block_index((*s).inodes[k]);       // finding the index of blockpointer of the current inode 
          (*s).inodes[k].blockptrs[fblock_index] = f_index;     // blockpointer stores the index of the datablock with alphabets corresponding to the filesize
          printf("the block index %d of the inode %s contains: %d \n", fblock_index, (*s).inodes[k].name, (*s).inodes[k].blockptrs[fblock_index]);
          (*s).freeblocklist[f_index + 1] = '1';                // to indicate that this index of datablock is in use
          printf("data block[%d] contains: %s \n", f_index, d_block[f_index]->index);
          break;
        }
      }
    }

    previous = parent;     // previous will remain -1 if the current inode is not a parent, otherwise will contain the inode number of the parent
    parent = -1;           // changing its value to -1 for the next iteration
  }
}

// copy file
void copyfile (char * sourcefile, char * destfile, superblock * s, datablock * d_block[]){
  char *path[TOTAL_BLOCKPTRS];                   
  char *single = strtok(sourcefile, "/");
  int i = 0;

  while (single != NULL) {
    path[i++] = single;
    single = strtok(NULL, "/");
  }

  int s_size;                                               // variable to store the size of the file
  for (int k = 0; k < TOTAL_INODES; k++) {
    if (strcmp((*s).inodes[k].name,path[i-1]) == 0) {       // finding the inode of the file to be copied
      if((*s).inodes[k].dir == 1) {
        printf("Can not handle directories.\n");
        exit(1);
      }
      s_size = (*s).inodes[k].size;                         // storing its size
      break;
    }
  }
  printf("this is the size of source file: %d \n", s_size);
  char str_size[32];                
  sprintf(str_size, "%d", s_size);               // changing the data type of size to pass it into the create file function
  printf("converted size %s \n", str_size);
  createfile(destfile, str_size, s, d_block);    // the create file function creates a copy                                 
}

// remove/delete fileed
void removefile (char * file, superblock * s, datablock * d_block[]) {
  char *path[TOTAL_BLOCKPTRS];             
  char *single = strtok(file, "/");
  int i = 0;

  while (single != NULL) {
    path[i++] = single;
    single = strtok(NULL, "/");
  }

  char * filename = path[i-1];
  printf("this is the filename: %s \n", filename);

  if (exists(filename, s) == -1){
    printf("The file does not exist.\n");        // error checking in case the file does not exist
    exit(1);
  }

  int value = i;
  int file_inode;                                       
  for (int i = 0; i < TOTAL_INODES; i++){
    if (strcmp((*s).inodes[i].name, filename) == 0){       // finding the inode of the file to be deleted
      file_inode = i;                                      // storing the inode number
      break;
    }
  }
  printf("this is the file inode: %d \n", file_inode); 
  int filesize = (*s).inodes[file_inode].size;             // storing the filesize of the current inode

  //subtracting directory size in accordance with the file to be deleted
  int the_inode = file_inode;  
  int escape = 0;                    
  for (int index = 0; index < value; index++){     // looping over all of the parents of file to be deleted
    int data_index;
    for (int g = 0; g < TOTAL_INODES; g++){
      for (int h = 0; h < TOTAL_BLOCKPTRS; h++){
        data_index = (*s).inodes[g].blockptrs[h];                 // finding the index of the data block of each of the block pointers of each inode     
        if (atoi(d_block[data_index]->index) == the_inode){       // if the index of datablock contains the current inode it means we have found the parent
          (*s).inodes[g].size = (*s).inodes[g].size - filesize;   // changing the size of the parent
          printf("this is the size of %s: %d \n", (*s).inodes[g].name, (*s).inodes[g].size);
          the_inode = g;       // changing the current inode for next iteration
          escape = 1;
          break;
        }
      }
      if (escape == 1){
        break;               // exiting the outerloop
      }
    }
  }

  if (escape == 0){      // escape would only be 0 if we did not find the parent
    printf("The directory at the given path does not exist.\n");       // parent not found means that there is incorrect path given
    exit(1);
  }

  int leave = 0;
  int data_index;
  for (int i = 0; i < TOTAL_INODES; i++){
    for (int j = 0; j < TOTAL_BLOCKPTRS; j++){
      data_index = (*s).inodes[i].blockptrs[j];    // finding the index of the data block of each of the block pointers of each inode     
      if (atoi(d_block[data_index]->index) == file_inode){
        printf("this is the data index which contains the inode: %d \n", data_index);
        printf("name of parent inode: %s and its blockpointer: %d \n", (*s).inodes[i].name, j);
        for (int l = 0; l < 1024; l++) {
          d_block[data_index]->index[l] = '\0';      // copy '\0' to each element of the datablock index
        }
        (*s).freeblocklist[data_index + 1] = '0';    // to show that this index of datablock is available for some other data
        (*s).inodes[i].blockptrs[j] = -1;            // to indicate that this index of the blockpointer of the inode is no longer pointing to a datablock index  
	      leave = 1;
        break;
      }
    }

    if (leave == 1){
      break;
    }
  }

  int d_block_index;
  for (int k = 0; k < TOTAL_BLOCKPTRS; k++){
    if ((*s).inodes[file_inode].blockptrs[k] != -1){              // checking if the block pointers of the file to be deleted are pointing towards any data block index
      printf("this is the file inode: %d \n", file_inode);
      d_block_index = (*s).inodes[file_inode].blockptrs[k];       // if they are pointing towards some index then storing that index
      printf("this is the data block index with data from file1: %d\n", d_block_index);
      for (int l = 0; l < 1024; l++) {
        d_block[d_block_index]->index[l] = '\0'; // Copy "" to each element of the index array
      }
      printf("datablock[%d] contains: %s \n", d_block_index, d_block[d_block_index]->index);
      (*s).freeblocklist[d_block_index + 1] = '0';                // updating the freeblocklist index to show that it is available for some other data
      (*s).inodes[file_inode].blockptrs[k] = -1;                  // setting the blockpointer of the file to be deleted to -1 so that it does not point towards any data index
    }
  }

  // setting all the values of that inode 
  (*s).inodes[file_inode].used = 0;
  (*s).inodes[file_inode].rsvd = 0;
  strcpy(s->inodes[file_inode].name, "");
  (*s).inodes[file_inode].dir = 0;
  (*s).inodes[file_inode].size = 0;

  

}

// move a file
void movefile (char * source, char * destination, superblock * s, datablock * d_block[]) {
  // parsing the source 
  char *path[8];                              
  char *single = strtok(source, "/");
  int i = 0;

  while (single != NULL) {
    path[i++] = single;
    single = strtok(NULL, "/");
  }

  // parsing the destination
  char *path2[8];                              
  char *single2 = strtok(destination, "/");
  int m = 0;

  while (single2 != NULL) {
    path2[m++] = single2;
    single2 = strtok(NULL, "/");
  }

  int num;  // to store the inode of the file
  for (int k = 0; k < TOTAL_INODES; k++) {                             
    if (strcmp((*s).inodes[k].name,path[i-1]) == 0) {      // finding the inode of the file to be moved
      num = k;
      break;
    }
  }

  if((*s).inodes[num].dir == 1) {
    printf("Can not handle directories.\n");    // since we have to move only files
    exit(1);
  }

  int parent;                                     // to store the inode of parent of the file to be moved
  for (int l = 0; l < TOTAL_INODES; l++) {
    if (exists(path[i-2], s) == -1) {
      printf("The directory %s at the source path does not exist.\n", path[i-2]);      // in case of invalid path in the source 
      return;
    } else if (strcmp((*s).inodes[l].name,path[i-2]) == 0) {     // finding the previous parent of the file to be moved
      parent = l;
      printf("found: %s \n", s->inodes[parent].name);
      break;
    }
  }

  printf("this is the name: %s \n", (*s).inodes[num].name);
  printf("this is the parent of source file: %s \n", s->inodes[parent].name);

  int parent2;                                     // finding inode of parent2 in which the file is to be moved
  for (int l = 0; l < TOTAL_INODES; l++) {
    if (exists(path2[i-2], s) == -1) {             
      printf("The directory %s at the destination path does not exist.\n", path2[i-2]);    // in case of invalid path in the destination
      return;
    } else if (strcmp((*s).inodes[l].name,path2[i-2]) == 0) {      // finding the new parent of the file to be moved
      parent2 = l;
      printf("found: %s \n", s->inodes[parent2].name);
      break;
    }
  }

  // converting the inode number to char 
  char inode_num[20];
  sprintf(inode_num, "%d", num);
  inode_num[strlen(inode_num)] = '\0';

  for (int b = 0; b < TOTAL_BLOCKPTRS; b++) {
    int d_index = (*s).inodes[parent].blockptrs[b];              // finding the datablock index which contains the inode number of the current inode
    if (strcmp(d_block[d_index]->index , inode_num) == 0) {
      int b_index = block_index((*s).inodes[parent2]);           // finding the blockpointer index of the new parent which would point towards the datablock index
      (*s).inodes[parent2].blockptrs[b_index] = (*s).inodes[parent].blockptrs[b];    // the new parent is now pointing towards the datablock index with inode number of current inode
      (*s).inodes[parent].blockptrs[b] = -1;          // the blockpointer of the parent is no longer pointing towards the datablock index with the current inode number
    }
  }
}

// list file info
void listfile (superblock * s) {
  for (int i = 0; i < TOTAL_INODES; i++){
    if (s->inodes[i].used == 1){                 // selecting the inodes which are in use
      if(s->inodes[i].dir == 1){                 // checking if it is a file or a directory
        printf ("Directory: %s Directory size: %d \n", s->inodes[i].name, s->inodes[i].size);     // printing name and size of the directory
      } else if (s->inodes[i].dir == 0) {
        printf ("File name: %s File size: %d \n", s->inodes[i].name, s->inodes[i].size);    // printing name and size of the file
      }
    }
  }
}

// create directory
void createdirectory (char * directory, superblock * s, datablock * d_block[]) {
  //seperate condition for creation of root directory as we cannot parse using '/' because then root directory is '/'
  if (strcmp(directory, "/") == 0){
    (*s).inodes[0].used = 1;
    (*s).inodes[0].dir = 1;
    strcpy((*s).inodes[0].name, directory);
    (*s).inodes[0].rsvd = 0;
    (*s).inodes[0].size = 0;
    return;
  }

  char *path[TOTAL_BLOCKPTRS];        
  char *single = strtok(directory, "/");
  int i = 0;

  while (single != NULL) {
    path[i++] = single;
    single = strtok(NULL, "/");
  }

  if(strlen(path[i-1]) > FILENAME_MAXLEN) {                       // path[i-1] is the name of the directory to be created
    printf("Name of the file cannot exceed 8 characters.\n");     // exiting if length of the name exceeds 8 characters
    exit(1);
  }

  if(exists(path[i-1],s) != -1) {
    printf("The directory already exists.\n");
    exit(1);                                       // exiting if the file already exists in the inode table
  }

  int previous = -1;      // variable to store the previous element of the path

  for (int j = 0; j < i; j++) {
    printf("Element in create directory: %d: %s\n", j, path[j]);
    printf("Parent value in create directory: %d", exists(path[j],s));
    printf("\n");
    int parent = -1;                 // variable to check if the current element is a parent directory
    parent = exists(path[j],s);      // using helper function to check
    if ((*s).inodes[0].blockptrs[0] ==  -1) {     // if the root is not pointing towards anything then it should point towards the current inode
      previous = 0;                               // parent of the current inode is root
    }

    for (int k = 0; k < TOTAL_INODES; k++){
      if ((*s).inodes[k].used == 0) {       // running a loop over all 16 inodes to find the next empty inode

        if (previous != -1 && parent == -1){     // if the previous directory in the path is the parent directory of the current directory and the current directory does not already exist
          if (path[j] != path[i-1]) {
            printf("The directory %s at the given path does not exist.\n", path[j]);    // error checking for wrong path
            exit(1);
          }
          int d_index = data_index(s);          // finding the free index in the datablock
          printf ("this is d_index in create directory: %d \n", d_index);
          int b_index = block_index((*s).inodes[previous]);                  // finding free index of the blockpointer of the inode
          printf ("this is b_index in create directory: %d \n", b_index);
          if (d_index != -1 && b_index != -2) {        // if we have space both in the data block and the block pointer
            char inode_num[20];                                          // converting inode num to char to store in the datablock
            sprintf(inode_num, "%d", k);
            inode_num[strlen(inode_num)] = '\0';                         // null-terminating
            printf("this is converted to char: %s \n", inode_num);
            strcpy(d_block[d_index]->index , inode_num);                 // storing the inode num to datablock
            printf("printing the content of data block: %s \n", d_block[d_index]->index);
            (*s).freeblocklist[d_index + 1] = '1';                 // updating the freeblock index because this index of the datablock is not occupied
            (*s).inodes[previous].blockptrs[b_index] = d_index;    // blockpointer of the parent directory is pointing towards the datablock in which inode of the current directory is stored
            printf("%d",(*s).inodes[previous].blockptrs[b_index]);
            printf(" is the value stored in the blockpointer %d of %d inode in create directory \n", b_index, previous);
          } else if (d_index == -1){                           // if there is no space in data block
            printf ("No Space left in the data block \n");
            exit(1);
          } else if (b_index == -2){                           // when all 8 block pointers of the parent directory are pointing towards an index of data block
            printf ("You cannot create more than 8 sub directories \n");
            exit(1);
          }
        }

        // if the current directory is new then changing attributes of inode
        if (parent == -1) {      
          (*s).inodes[k].used = 1;
          (*s).inodes[k].dir = 1;
          (*s).inodes[k].size = 0;
          strcpy((*s).inodes[k].name, path[j]);
          (*s).inodes[k].rsvd = 0;
          break;
        }
      }
    }

    previous = parent;      // making current node the parent for next iteration
    parent = -1;
  }
}

// remove a directory
void removedirectory (char * directory, superblock * s, datablock * d_block[]) {
  char *path[8];                              
  char *single = strtok(directory, "/");
  int i = 0;

  while (single != NULL) {
    path[i++] = single;
    single = strtok(NULL, "/");
  }

  char * name = path[i-1];
  printf("this is the directory name: %s \n", name);

  if (exists(name, s) == -1){
    printf("The directory does not exist.\n");        // error checking in case the directory does not exist
    exit(1);
  }

  int d_inode;
  for (int i = 0; i < TOTAL_INODES; i++){
    if (strcmp((*s).inodes[i].name, name) == 0){
      d_inode = i;                                      // storing the inode number of the directory to be deleted
      break;
    }
  }
  printf("this is the directory inode: %d \n", d_inode);  

  int escape = 0;
  int data_index;
  for (int i = 0; i < TOTAL_INODES; i++){
    for (int j = 0; j < TOTAL_BLOCKPTRS; j++){
      data_index = (*s).inodes[i].blockptrs[j];             // finding the index of the data block of each of the block pointers of each inode    
      if (atoi(d_block[data_index]->index) == d_inode){
        printf("this is the data index which contains the inode %d \n", data_index);
        printf("name of parent inode: %s and its blockpointer: %d \n", (*s).inodes[i].name, j);
        for (int l = 0; l < 1024; l++) {
          d_block[data_index]->index[l] = '\0';     // copying '\0' to each element of the index array
        }
        (*s).freeblocklist[data_index + 1] = '0';   // this index is now available for some other data            
        (*s).inodes[i].blockptrs[j] = -1;           // the blockpointer of the parent directory is no longer pointing to the data index which contained the inode of the directory to be deleted   
	      escape = 1;
        break;
      }
    }
    if(escape == 1){
      break;
    }
  } 

  if (escape == 0){      // escape would only be 0 if we did not find the parent
    printf("The directory at the given path does not exist.\n");       // parent not found means that there is incorrect path given
    exit(1);
  }

  int d_block_index;
  for (int k = 0; k < TOTAL_BLOCKPTRS; k++){
    if ((*s).inodes[d_inode].blockptrs[k] != -1){              // checking if the blockpointers of the directory to be deleted are pointing towards any data index
      d_block_index = (*s).inodes[d_inode].blockptrs[k];       // storing that data index
      for (int l = 0; l < 1024; l++) {
        d_block[d_block_index]->index[l] = '\0';               // copying '\0' to each element of the index array
      }
      (*s).freeblocklist[d_block_index + 1] = '0';             // setting this index of freeblocklist to 0 as it is now available for some other data
      (*s).inodes[d_inode].blockptrs[k] = -1;                  // the blockpointer of the directory to be deleted is not pointing towards anything now 
    }
  }

  // setting up the inode values of the directory to be deleted
  (*s).inodes[d_inode].used = 0;
  (*s).inodes[d_inode].rsvd = 0;
  strcpy(s->inodes[d_inode].name, "");
  (*s).inodes[d_inode].dir = 0;
  (*s).inodes[d_inode].size = 0;
}

// main
int main (int argc, char* argv[]) {
  void * disk = malloc(128*1024);               // allocating 128 KB for the disk on heap
  superblock * s_block = (superblock*) disk;    // superblock is the first 1KB of the disk
  datablock * d_block[127];                     // datablock is the rest 127 KB of the disk
  for (int i = 0; i < 127; i++) { 
    d_block[i] = (datablock*)((char *)(disk + 1024) + (i * 1024));   // assigning 1 KB blocks to each element of d_block on the disk
  }

  set_null (s_block, d_block);       // setting the initial values of super block and data blocks 

  createdirectory("/", s_block, d_block);   // creating root directory before reading any command

  // while not EOF
  FILE * stream = fopen ("sampleinput.txt", "r");

  // read command
  char line[1024];
  while (fgets(line, sizeof(line), stream) != NULL) {
    char command[4];           // will contain the command
    char argument1[100];       // will contain the first argument
    char argument2[100];       // will contain the second argument

    // parse command
    sscanf(line, "%s %s %s", command, argument1, argument2);

    // call appropriate function
    if (strcmp(command, "CR") == 0){
      if (check_space(s_block) == -1){                // if there is no space to create a file 
        printf("No Space to create a file.\n");       // printing the error message and exiting the program
        exit(1);
      } else {
        createfile(argument1, argument2, s_block, d_block);
      }
    } else if (strcmp(command, "CP") == 0) {
      if (check_space(s_block) == -1){
        printf("No Space to copy a file.\n");         // if there is no space to copy a file
        exit(1);
      } else {
        copyfile(argument1, argument2, s_block, d_block);
      }
    } else if(strcmp(command, "DL") == 0) {
      removefile(argument1, s_block, d_block);
    } else if (strcmp(command, "MV") == 0) {
      movefile(argument1, argument2, s_block, d_block);
    } else if (strcmp(command, "LL") == 0) {
      listfile(s_block);
    } else if (strcmp(command, "CD") == 0) {
      if (check_space(s_block) == -1){
        printf("No Space to create directory.\n");     // if there is no space to create a directory
        exit(1);
      } else {
        createdirectory(argument1, s_block, d_block);
      }
    } else if (strcmp(command, "DD") == 0) {
      removedirectory(argument1, s_block, d_block);
    }
  }

  const char *filename = "my_fs.txt";     // filename 
  int filesize = 128 * 1024;              // 128 KB
  FILE *file = fopen(filename, "wb");     

  char *buffer = (char *)malloc(filesize);             // creating buffer on heap
  if (buffer == NULL) {
    perror("Failed to allocate memory for buffer");    // to handle allocation failure
    fclose(file);
    return 1;
  }
  memset(buffer, '\0', filesize);          // initializing the buffer
  fwrite(buffer, filesize, 1, file);       // writing data to the file
  fseek(file, 0, SEEK_SET);                // pointing towards the beginning of the file

  // writing superblock 
  if (fwrite(s_block, (128 * sizeof(char)) + (8 * sizeof(inode)), 1, file) != 1) {
    perror("Unable to write data to the file.\n");      // in case of failure
    fclose(file);
    return 1;
  }

  // writing datablock
  for (int c = 0; c < 127; c++) {
    if (fwrite(d_block[c]->index, 1024, 1, file) != 1) {
      perror("Unable to write data to the file.\n");        // in case of failure
      fclose(file);
      return 1;
    }
  }

  // closing both files
  fclose(file);
  fclose(stream);

  // to free the memory allocated on heap
  free (buffer);
  free (disk);   
	return 0;
}
