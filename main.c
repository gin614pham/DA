#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

// Function to print file information
void printFileInfo(const char *fileName) {
    struct stat fileInfo;

    // Get file information
    if (stat(fileName, &fileInfo) < 0) {
        perror("stat");
        return;
    }

    // Check if it's a directory
    if (S_ISDIR(fileInfo.st_mode)) {
        printf("Directory: %s\n", fileName);
    } else if (S_ISREG(fileInfo.st_mode)) {
        printf("File: %s\n", fileName);
    } else {
        printf("Other: %s\n", fileName);
    }

    printf("Number of Links: %ld\n", (long)fileInfo.st_nlink);
    printf("File Permissions: ");
    printf((S_ISDIR(fileInfo.st_mode)) ? "d" : "-");
    printf((fileInfo.st_mode & S_IRUSR) ? "r" : "-");
    printf((fileInfo.st_mode & S_IWUSR) ? "w" : "-");
    printf((fileInfo.st_mode & S_IXUSR) ? "x" : "-");
    printf((fileInfo.st_mode & S_IRGRP) ? "r" : "-");
    printf((fileInfo.st_mode & S_IWGRP) ? "w" : "-");
    printf((fileInfo.st_mode & S_IXGRP) ? "x" : "-");
    printf((fileInfo.st_mode & S_IROTH) ? "r" : "-");
    printf((fileInfo.st_mode & S_IWOTH) ? "w" : "-");
    printf((fileInfo.st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");

    struct passwd *pw = getpwuid(fileInfo.st_uid);
    struct group *gr = getgrgid(fileInfo.st_gid);
    printf("Owner: %s\n", pw ? pw->pw_name : "Unknown");
    printf("Group: %s\n", gr ? gr->gr_name : "Unknown");

    printf("Size: %ld bytes\n", (long)fileInfo.st_size);

    // Format and print the last access time
    char buffer[80];
    struct tm *timeinfo = localtime(&fileInfo.st_atime);
    strftime(buffer, 80, "Last Access Time: %Y-%m-%d %H:%M:%S", timeinfo);
    printf("%s\n", buffer);

    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [file/directory1] [file/directory2] ...\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        printFileInfo(argv[i]);
    }

    return 0;
}
