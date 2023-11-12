#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>
#include <sys/wait.h>

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
    printf("Blocks: %ld\n", (long)fileInfo.st_blocks);
    printf("Block Size: %ld bytes\n", (long)fileInfo.st_blksize);

    // Format and print the last access time
    char buffer[80];
    strftime(buffer, 80, "Last Access Time: %Y-%m-%d %H:%M:%S", localtime(&fileInfo.st_atime));
    printf("%s\n", buffer);

    printf("\n");
}

void createFolder(const char *folderName) {
    // create a folder
    if (mkdir(folderName, 0777) == 0) {
        printf("Folder '%s' created successfully.\n", folderName);
    } else {
        perror("mkdir");
    }
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

void changePermission(const char *fileName, mode_t permission) {
    // change permission
    if (chmod(fileName, permission) == 0) {
        printf("Permission of '%s' changed to %o successfully.\n" , fileName , permission);
    } else {
        perror("chmod");
    }
}

void changeOwnerAndGroup(const char *file_path, const char *user_name, const char *group_name) {
    // check run as root
    if (getuid() != 0) {
        fprintf(stderr, "To change owner and group, please run program as root\n");
        exit(EXIT_FAILURE);
    }
    uid_t uid = -1; 
    gid_t gid = -1; 

    if (user_name != NULL) {
        struct passwd *pwd = getpwnam(user_name);
        if (pwd == NULL) {
            perror("Failed to get uid");
            exit(EXIT_FAILURE);
        }
        uid = pwd->pw_uid;
    }

    if (group_name != NULL) {
        struct group *grp = getgrnam(group_name);
        if (grp == NULL) {
            perror("Failed to get gid");
            exit(EXIT_FAILURE);
        }
        gid = grp->gr_gid;
    }

    if (chown(file_path, uid, gid) == -1) {
        perror("chown failed");
        exit(EXIT_FAILURE);
    } else {
        printf("Owner and group of '%s' changed to '%s:%s' successfully.\n", file_path, user_name, group_name);
    }
}


void mergeFile(const char *fileName, const char *fileName2) {
    int fd1[2];
    int fd2[2];

    FILE *fp1 = fopen(fileName, "r");
    FILE *fp2 = fopen(fileName2, "r");
    createFile("file3.txt");
    FILE *fp3 = fopen("file3.txt", "w");

    if (fp1 == NULL || fp2 == NULL) {
        perror("fopen");
        return;
    }

    if (pipe(fd1) == -1) {
        perror("pipe");
        return;
    }

    if (pipe(fd2) == -1) {
        perror("pipe");
        return;
    }

    pid_t p;
    p = fork();

    if (p < 0) {
        perror("fork");
        return;
    }

    //parent process
    else if (p > 0) {
        char concat_str[100];
 
        close(fd1[0]);

        while (fgets(concat_str, 100, fp1) != NULL) {
            write(fd1[1], concat_str, strlen(concat_str) + 1);
        }
        printf("P1 %s\n", concat_str);
        close(fd1[1]);

        wait(NULL);

        close(fd2[1]);

        read(fd2[0], concat_str, 100);
        printf("P2 %s\n", concat_str);
        fprintf(fp3, "%s", concat_str);
        close(fd2[0]);
    }
    //child process
    else {
        close(fd1[1]);
        char concat_str[100];
        char concat_str2[100];
        read(fd1[0], concat_str, 100);
        printf("C1 %s\n", concat_str);

        while (fgets(concat_str2, 100, fp2) != NULL) {
            int k = strlen(concat_str);
            for (int i = 0; i < strlen(concat_str2); i++) {
                concat_str[k++] = concat_str2[i];
            }
            concat_str[k] = '\0';
        }
        printf("C2 %s\n", concat_str);

        close(fd1[0]);
        close(fd2[0]);

        write(fd2[1], concat_str, strlen(concat_str) + 1);
        printf("C4 %s\n", concat_str);
        close(fd2[1]);

        exit(0);      
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
            printf("Usage: ./myFileManager -g file_name file_name2 to merge two files\n");
            printf("Usage: ./myFileManager -p file_name permission to change permission\n");
            printf("Usage: ./myFileManager -o file_name user_name group_name to change owner and group\n");
            printf("Usage: ./myFileManager -o -u file_name user_name to change owner\n");
            printf("Usage: ./myFileManager -o -g file_name group_name to change group\n");
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
            case 'g':
                if (argc != 4) {
                    printf("Invalid argument\n");
                    printf("Usage: ./myFileManager -mg file_name file_name2 to merge two files\n");
                }
                mergeFile(argv[2], argv[3]);
                break;
            case 'p':
                if (argc != 4) {
                    printf("Invalid argument\n");
                    printf("Usage: ./myFileManager -p file_name permission to change permission\n");
                    break;
                }
                mode_t permission = strtol(argv[3], NULL, 8);
                changePermission(argv[2], permission);
                break;
            case 'o':
                if (argc != 5) {
                    printf("Invalid argument\n");
                    printf("Usage: ./myFileManager -o file_name user_name group_name to change owner and group\n");
                    printf("Usage: ./myFileManager -o -u file_name user_name to change owner\n");
                    printf("Usage: ./myFileManager -o -g file_name group_name to change group\n");
                    break;
                }
                if (strcmp(argv[2], "-u") == 0) {
                    changeOwnerAndGroup(argv[3], argv[4], NULL);
                } else if (strcmp(argv[2], "-g") == 0) {
                    changeOwnerAndGroup(argv[3], NULL, argv[4]);
                } else {
                    changeOwnerAndGroup(argv[2], argv[3], argv[4]);
                }
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
                printf("Usage: ./myFileManager -g file_name file_name2 to merge two files\n");
                printf("Usage: ./myFileManager -p file_name permission to change permission\n");
                printf("Usage: ./myFileManager -o file_name user_name group_name to change owner and group\n");
                printf("Usage: ./myFileManager -o -u file_name user_name to change owner\n");
                printf("Usage: ./myFileManager -o -g file_name group_name to change group\n");
                break;
        }
    } else {
        // print help
        printf("Usage: ./myFileManager -help or ./myFileManager -h to show help\n");
    }

    return 0;
}