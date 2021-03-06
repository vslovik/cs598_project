#include <stdio.h>
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <stdlib.h>
#include <sys/time.h>

double lastTime;

double getTime() {
  timeval now;
  gettimeofday(&now, NULL);
  return ((double) now.tv_sec) + ((double) now.tv_usec)/1000000.;
}
void start () {
  lastTime = getTime();
} 

double stop () {
  double d = (getTime()-lastTime);
  return d;
} 

int compare (const void * a, const void * b)
{
   return ( *(char*)a - *(char*)b );
}

int main(int argc, char * argv[])
{
	FILE * fp;
	FILE * writer;

	if(argc < 2)
	{
		std::cout << "Please provide file to sort on. ( ./externalMergeSort example.txt )\n";
		return 0;
	}

	fp = fopen(argv[1], "r+");

	if( fp == NULL ) 
   {
      perror ("Error opening file");
      return(-1);
   }

   fseek(fp, 0, SEEK_END);
   int len = ftell(fp); //length of file in bytes
   fseek(fp, 0, SEEK_SET);

   //Assume for now that file length is 1 GB (1,000,000,000 bytes). Let's just split it up into 100 MB each read
   //Testing on a 1k kB file

   int blockSize = len / 10; // Later we can fix this number
   //int blockSize = 100000000; 
   int numberOfBlocks = len / blockSize;
   //std::vector <char> buffer(blockSize);
   //std::vector <char> writeBuffer(blockSize);

   char * buffer = (char *)malloc(blockSize + 1);
   char * writeBuffer = (char *)malloc(blockSize + 1);

   writeBuffer[0] = 'r';
   writeBuffer[blockSize -1 ] = '\0';

   std::cout << len << " " << blockSize << " " << numberOfBlocks << std::endl;

   int x;
   int iter = 0;

   start();
   
   while((x = fread(buffer, sizeof(char), blockSize, fp)) > 0)
   {
	  	qsort(buffer, blockSize, sizeof(char), compare);
	  	char * write = (char *)&buffer[0];

	  	fseek(fp, iter * blockSize, SEEK_SET);
	  	fwrite( write, sizeof(char), blockSize, fp);
	  	fseek(fp, (++iter)*blockSize, SEEK_SET);
   }

   fclose(fp);

   fp = fopen(argv[1], "r+");
   writer = fopen("temp.txt", "w");

   int * indexes = (int *)malloc(sizeof(int) * numberOfBlocks); // Lets use know when to pull in more blocks
   int * blockCount = (int *)malloc(sizeof(int) * numberOfBlocks); // How many sections of each block have we used
   int shardSize = blockSize / numberOfBlocks;
   int count = 0;
   // Load parts of the blocks into memory to sort

   for (int i = 0; i < numberOfBlocks; i++)
   {
   	indexes[i] = 0;
   	blockCount[i] = 0;
   	fseek(fp, i*blockSize, SEEK_SET);
   	fread( &buffer[i*shardSize], sizeof(char), shardSize, fp);
   }

   int writeBufferCount = 0;

   while (count < len)
   {
     int found = -1;
     char min = ' ';
     for (int j = 0; j < numberOfBlocks; j++)
     {
       if ( (found == -1 || (int)buffer[j*shardSize + indexes[j]] < (int)min) && (indexes[j] < shardSize) )
       {
         found = j;
         min = buffer[j*shardSize + indexes[j]];
       }  
     }    

     writeBuffer[writeBufferCount] = buffer[found*shardSize + indexes[found]];

     //printf("%d WB = %s %c\n", found, (char *)&writeBuffer[0], buffer[found*shardSize + indexes[found]]);
     //fwrite((void *)&buffer[found*shardSize + indexes[found]], sizeof(char), 1, writer);
     indexes[found]++;
     writeBufferCount++;

     if (indexes[found] >= shardSize)
     {
       blockCount[found]++;
       fseek(fp, blockSize*found + shardSize*blockCount[found], SEEK_SET);

       if(blockCount[found] < blockSize/shardSize)
       {
	       fread( &buffer[found*shardSize], sizeof(char), shardSize, fp);
	       indexes[found] = 0;
	     }
     }

     if (writeBufferCount >= blockSize)
     {
      //printf("%d WB = %s\n", writeBufferCount, (char *)&writeBuffer[0]);
      fwrite(writeBuffer, sizeof(char), blockSize, writer);
      writeBufferCount = 0;
     }
     count++;

   }

   free(indexes);
   free(blockCount);
   free(buffer);
   free(writeBuffer);
    printf("Time = %.4f\n", stop());
}
