#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <dirent.h>
#include <locale.h>
#include <string.h>

#define SYMBOLIC_T 0b1000
#define DIR_T 0b0100
#define FILE_T 0b0010
#define LC_COLLATE_T 0b0001

int getopt(int argc, char * const argv[],
	const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

void printFile(char* str, struct dirent *myfile, int deep)
{
     for(int i = deep; i >= 0; i--)
     {
          printf("\t");
     }
     char buf[512];
     sprintf(buf, "%s/%s", str, myfile->d_name);
     struct stat mystat;
     stat(buf, &mystat);
     printf("%s ", myfile->d_name);
     printf("%zu ",mystat.st_size);
     switch (myfile->d_type)
     {
          case DT_LNK:
               printf("Symbol link ");
          break;
          case DT_DIR:
               printf("Directory ");
          break;
          case DT_REG:
               printf("File ");
          break;
     }
     printf("\n");

}

void walkDir(char* str, char options, int deep)
{
     DIR *mydir;
     struct dirent *myfile;

     mydir = opendir(str);

     if(mydir == NULL)
     {
          return;
     }

     while((myfile = readdir(mydir)) != NULL)
     {
          if((strcmp(myfile->d_name, ".") == 0) || (strcmp(myfile->d_name, "..") == 0))
          {
               continue;
          }

          if((options & SYMBOLIC_T) && (DT_LNK == myfile->d_type))
          {
               printFile(str, myfile, deep);
          }
          if((options & DIR_T) && (DT_DIR == myfile->d_type))
          {
               printFile(str, myfile, deep);
          }
          if(myfile->d_type == DT_DIR)
          {
               char* nextDirectori = malloc(512);
               strcpy(nextDirectori, str);
               strcat(nextDirectori, "/");
               strcat(nextDirectori, myfile->d_name);
               walkDir(nextDirectori, options, deep + 1);
               free(nextDirectori);
          }
          if((options & FILE_T) && (DT_REG == myfile->d_type))
          {
               printFile(str, myfile, deep);
          }
     }
     closedir(mydir);
}

int main( int argc, char* argv[] )
{
     int option = 0;

     char def_options = SYMBOLIC_T | DIR_T | FILE_T;
     char current_options = 0;

     char* dirString = "\n";

	while ( (option = getopt(argc, argv, "ldfs")) != -1){
		switch (option) {
		case 'l': current_options |= SYMBOLIC_T; break;
		case 'd': current_options |= DIR_T; break;
		case 'f': current_options |= FILE_T; break;
		case 's': current_options |= LC_COLLATE_T; break;
		}
	}

     if(current_options == 0)
     {
          current_options = def_options;
     }
     for(int i = argc - 1; i >= 0; i--)
     {
          if(*(argv[i]) != '-')
          {
               dirString = argv[i];
               break;
          }
     }

     if(dirString[0] == '\n')
     {
          strcpy(dirString, "..\n");
     }
     
     walkDir(dirString, current_options, 0);

     return 0;
}