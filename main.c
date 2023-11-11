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
#include <dirent.h>

// Function to print file information
void printFileInfo(const char *filePath) {
    const char *fileName = strrchr(filePath, '/') ? strrchr(filePath, '/') + 1 : filePath;

    struct stat fileInfo;

    // Get file information
    if (stat(filePath, &fileInfo) < 0) {
        perror("stat");
        return;
    }
    printf("Information for '%s':\n", fileName);

    // Check if it's a directory
    printf("%s: %s\n", S_ISDIR(fileInfo.st_mode) ? "Directory" : "File", fileName);

    printf("Number of Links: %ld\n", (long)fileInfo.st_nlink);
    printf("File Permissions: %o\n", fileInfo.st_mode & 0777);

    struct passwd *pw = getpwuid(fileInfo.st_uid);
    struct group *gr = getgrgid(fileInfo.st_gid);
    printf("Owner: %s\n", pw ? pw->pw_name : "Unknown");
    printf("Group: %s\n", gr ? gr->gr_name : "Unknown");

    printf("Size: %ld bytes\n", (long)fileInfo.st_size);

    // Format and print the last access time
    char buffer[80];
    strftime(buffer, 80, "Last Access Time: %Y-%m-%d %H:%M:%S", localtime(&fileInfo.st_atime));
    printf("%s\n", buffer);

    printf("\n");
}

void createFile(const char *fileName) {
    // create a file
    FILE *fp = fopen(fileName, "w");
    if (fp != NULL) {
        fclose(fp);
        printf("File '%s' created successfully.\n", fileName);
    } else {
        perror("fopen");
    }
}

void deleteFile(const char *fileName) {
    // delete a file
    if (remove(fileName) == 0) {
        printf("File '%s' deleted successfully.\n", fileName);
    } else {
        perror("remove");
    }
}

void renameFile(const char *oldName, const char *newName) {
    // rename a file
    if (rename(oldName, newName) == 0) {
        printf("File '%s' renamed to '%s' successfully.\n", oldName, newName);
    } else {
        perror("rename");
    }
}

void moveFile(const char *source, const char *destination) {
    // move a file
    if (rename(source, destination) == 0) {
        printf("File '%s' moved to '%s' successfully.\n", source, destination);
    } else {
        perror("rename");
    }
}

void listFiles(const char *directoryPath) {
    // list files in a directory
    DIR *dir = opendir(directoryPath);
    if (dir != NULL) {
        struct dirent *entry;
        printf("Files in directory '%s':\n", directoryPath);
        while ((entry = readdir(dir)) != NULL) {
            printf("%s\n", entry->d_name);
        }
        closedir(dir);
    } else {
        perror("opendir");
    }
}

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        // check if the first argument is "-h" or "-help"
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0) {
            // print help message
            printf("Usage: ./myFileManager -c file_name to create a file\n");
            printf("Usage: ./myFileManager -d file_name to delete a file\n");
            printf("Usage: ./myFileManager -r old_file_name new_file_name to rename a file\n");
            printf("Usage: ./myFileManager -m source_file destination_file to move a file\n");
            printf("Usage: ./myFileManager -l directory_path to list files in a directory\n");
            printf("Usage: ./myFileManager -i file_name to show information about a file\n");
            printf("Usage: ./myFileManager -h or ./myFileManager -help to show help\n");
            return 0;
        }
        // switch on the first argument
        switch (argv[1][1]) {
            case 'c':
                // check if there are enough arguments
                if (argc != 3) {
                    printf("Invalid argument\n");
                    printf("Usage: ./myFileManager -c file_name to create a file\n");
                }
                createFile(argv[2]);
                break;
            case 'd':
                // check if there are enough arguments
                if (argc != 3) {
                    printf("Invalid argument\n");
                    printf("Usage: ./myFileManager -d file_name to delete a file\n");
                }
                deleteFile(argv[2]);    
                break;
            case 'r':
                // check if there are enough arguments
                if (argc != 4) {
                    printf("Invalid argument\n");
                    printf("Usage: ./myFileManager -r old_file_name new_file_name to rename a file\n");
                }
                renameFile(argv[2], argv[3]);
                break;
            case 'm':
                // check if there are enough arguments
                if (argc != 4) {
                    printf("Invalid argument\n");
                    printf("Usage: ./myFileManager -m source_file destination_file to move a file\n");
                }
                moveFile(argv[2], argv[3]);
                break;
            case 'l':
                listFiles(argc > 2 ? argv[2] : ".");
                break;
            case 'i':
                // check if there are enough arguments
                if (argc != 3) {
                    printf("Invalid argument\n");
                    printf("Usage: ./myFileManager -i file_name to show information about a file\n");
                }
                printFileInfo(argv[2]);
                break;
            default:
                printf("Invalid argument\n");
                printf("Usage: ./myFileManager -c file_name to create a file\n");
                printf("Usage: ./myFileManager -d file_name to delete a file\n");
                printf("Usage: ./myFileManager -r old_file_name new_file_name to rename a file\n");
                printf("Usage: ./myFileManager -m source_file destination_file to move a file\n");
                printf("Usage: ./myFileManager -l directory_path to list files in a directory\n");
                printf("Usage: ./myFileManager -i file_name to show information about a file\n");
                printf("Usage: ./myFileManager -h or ./myFileManager -help to show help\n");
                break;
        }
    } else {
        // print help
        printf("Usage: ./myFileManager -help or ./myFileManager -h to show help\n");
    }

    return 0;
}