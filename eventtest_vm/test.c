#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){

   char *string;
   size_t msgsize;
   
	
   msgsize = strlen("TestTestTest");
   printf("Size:%lu\n",msgsize);
   string = (char*) calloc(msgsize, sizeof(char));
   strncpy(string, "TestTestTest", msgsize);
   printf("Size of string:%lu\n",strlen(string));

   printf("%s\n", string);

   free(string);

   return 0;
}
