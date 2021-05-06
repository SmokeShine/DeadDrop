#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>



#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <ftw.h>
int main()
{
    printf("Hello world\n");
    /*Extract user name*/
    char user[256];

    /*Populate all files in current folder*/
    /*Filter files for user*/
    /*Find oldest file for the user*/
    /* From OSU study material */
    int oldestDirTime = INT_MAX;

    char targetDirPrefix[32] = ".c";
    static char oldestDirName[256];
    memset(oldestDirName, '\0', sizeof(oldestDirName));
    DIR *dirToCheck;
    struct dirent *fileInDir;
    struct stat dirAttributes;
    dirToCheck = opendir(".");
    if (dirToCheck > 0)
    {
        while ((fileInDir = readdir(dirToCheck)) != NULL)
        {
            if (strstr(fileInDir->d_name, targetDirPrefix) != NULL)
            {
                printf("%s\n",fileInDir->d_name);
                stat(fileInDir->d_name, &dirAttributes);
                if ((int)dirAttributes.st_mtime < oldestDirTime)
                {
                    
                    oldestDirTime = (int)dirAttributes.st_mtime;
                    memset(oldestDirName, '\0', sizeof(oldestDirTime));
                    strcpy(oldestDirName, fileInDir->d_name);
                }
            }
        }
    }
    printf("\t %s\n",oldestDirName);
    closedir(dirToCheck);

    return 0;
}