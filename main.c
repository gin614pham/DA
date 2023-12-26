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

// Function to print file information
void printFileInfo(const char *filePath)
{
    const char *fileName = strrchr(filePath, '/') ? strrchr(filePath, '/') + 1 : filePath;

    struct stat fileInfo;

    // Get file information
    if (stat(filePath, &fileInfo) < 0)
    {
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

void createFolder(const char *folderName)
{
    // create a folder
    if (mkdir(folderName, 0777) == 0)
    {
        printf("Folder '%s' created successfully.\n", folderName);
    }
    else
    {
        perror("mkdir");
    }
}

void createFile(const char *fileName)
{
    // create a file
    FILE *fp = fopen(fileName, "w");
    if (fp != NULL)
    {
        fclose(fp);
        printf("File '%s' created successfully.\n", fileName);
    }
    else
    {
        perror("fopen");
    }
}

void moveFileToTrash(const char *fileName)
{
    // Create file path
    char actualpath[PATH_MAX + 1];
    char *file_path = realpath(fileName, actualpath);

    // Create home path
    char *home_path = getenv("HOME");

    // Create path to trash
    char trash_path[PATH_MAX + 1];
    strcpy(trash_path, home_path);
    strcat(trash_path, "/.local/share/Trash/files/");
    strcat(trash_path, fileName);

    // Create path to trash info
    char trash_info_path[PATH_MAX + 1];
    strcpy(trash_info_path, home_path);
    strcat(trash_info_path, "/.local/share/Trash/info/");
    strcat(trash_info_path, fileName);
    strcat(trash_info_path, ".trashinfo");

    // Open file
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    // Open trash file
    FILE *trash_file = fopen(trash_path, "w");
    if (trash_file == NULL)
    {
        perror("fopen trash");
        return;
    }

    // Open trash info file
    FILE *trash_info_file = fopen(trash_info_path, "w");
    if (trash_info_file == NULL)
    {
        perror("fopen trash info");
        return;
    }

    // Read data from file
    char trash[1024];
    while (fgets(trash, sizeof(trash), file) != NULL)
    {
        // Write to trash file
        fputs(trash, trash_file);
    }

    // Get current time
    time_t time_value = time(NULL);
    struct tm *now = gmtime(&time_value);

    // Create time string
    char delete_at[500];
    strftime(delete_at, sizeof(delete_at), "%Y-%m-%dT%H:%M:%S", now);

    // Write info to infofile
    char trash_info[1024];
    sprintf(trash_info, "[Trash Info]\nPath=%s\nDeletionDate=%s", file_path, delete_at);
    fputs(trash_info, trash_info_file);

    // Close file
    fclose(file);
    remove(fileName);

    // Close trash_file
    fclose(trash_file);

    // Close trassh_info_file
    fclose(trash_info_file);

    printf("Moved file %s to trash.\n", fileName);
}

void deleteFile(const char *fileName)
{
    // delete a file
    if (remove(fileName) == 0)
    {
        printf("File '%s' deleted successfully.\n", fileName);
    }
    else
    {
        perror("remove");
    }
}

void renameFile(const char *oldName, const char *newName)
{
    // rename a file
    if (rename(oldName, newName) == 0)
    {
        printf("File '%s' renamed to '%s' successfully.\n", oldName, newName);
    }
    else
    {
        perror("rename");
    }
}

void moveFile(const char *source, const char *destination)
{
    // move a file
    if (rename(source, destination) == 0)
    {
        printf("File '%s' moved to '%s' successfully.\n", source, destination);
    }
    else
    {
        perror("rename");
    }
}

void listFiles(const char *directoryPath)
{
    // list files in a directory
    DIR *dir = opendir(directoryPath);
    if (dir != NULL)
    {
        struct dirent *entry;
        printf("Files in directory '%s':\n", directoryPath);
        while ((entry = readdir(dir)) != NULL)
        {
            printf("%s\n", entry->d_name);
        }
        closedir(dir);
    }
    else
    {
        perror("opendir");
    }
}

void changePermission(const char *fileName, mode_t permission)
{
    // change permission
    if (chmod(fileName, permission) == 0)
    {
        printf("Permission of '%s' changed to %o successfully.\n", fileName, permission);
    }
    else
    {
        perror("chmod");
    }
}

void changeOwnerAndGroup(const char *file_path, const char *user_name, const char *group_name)
{
    // check run as root
    if (getuid() != 0)
    {
        fprintf(stderr, "To change owner and group, please run program as root\n");
        exit(EXIT_FAILURE);
    }
    uid_t uid = -1;
    gid_t gid = -1;

    if (user_name != NULL)
    {
        struct passwd *pwd = getpwnam(user_name);
        if (pwd == NULL)
        {
            perror("Failed to get uid");
            exit(EXIT_FAILURE);
        }
        uid = pwd->pw_uid;
    }

    if (group_name != NULL)
    {
        struct group *grp = getgrnam(group_name);
        if (grp == NULL)
        {
            perror("Failed to get gid");
            exit(EXIT_FAILURE);
        }
        gid = grp->gr_gid;
    }

    if (chown(file_path, uid, gid) == -1)
    {
        perror("chown failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Owner and group of '%s' changed to '%s:%s' successfully.\n", file_path, user_name, group_name);
    }
}


void mergeFiles(const char *fileName, const char *fileName2)
{
    int fd1[2];
    int fd2[2];

    FILE *fp1 = fopen(fileName, "r");
    FILE *fp2 = fopen(fileName2, "r");

    createFile("file3.txt");
    FILE *fp3 = fopen("file3.txt", "w");


    if (fp1 == NULL || fp2 == NULL)
    {
        perror("fopen");
        return;
    }

    if (pipe(fd1) == -1)
    {
        perror("pipe");
        return;
    }

    if (pipe(fd2) == -1)
    {
        perror("pipe");
        return;
    }

    pid_t p;
    p = fork();

    if (p < 0)
    {
        perror("fork");
        return;
    }

    // parent process
    else if (p > 0)
    {
        char concat_str[100];

        close(fd1[0]);

        while (fgets(concat_str, 100, fp1) != NULL)
        {
            write(fd1[1], concat_str, strlen(concat_str) + 1);
        }
        printf("P1 %s\n", concat_str);
        close(fd1[1]);

        wait(NULL);

        close(fd2[1]);

        read(fd2[0], concat_str, 100);
        printf("P2 %s\n", concat_str);
        close(fd2[0]);
    }
    // child process
    else
    {
        close(fd1[1]);
        char concat_str[100];
        char concat_str2[100];
        read(fd1[0], concat_str, 100);
        printf("C1 %s\n", concat_str);

        while (fgets(concat_str2, 100, fp2) != NULL)
        {
            int k = strlen(concat_str);
            for (int i = 0; i < strlen(concat_str2); i++)
            {
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

void printHelp() {
	printf("Usage: ./myFileManager -c <file_name> to create a file\n");
	printf("Usage: ./myFileManager -d <file_name> to delete a file\n");
	printf("Usage: ./myFileManager -r <old_file> <new_file> to rename a file\n");
	printf("Usage: ./myFileManager -m <source_file> <destination_file> to move a file\n");
	printf("Usage: ./myFileManager -l <directory_path> to list files in a directory\n");
	printf("Usage: ./myFileManager -i <file_name> to show information about a file\n");
	printf("Usage: ./myFileManager -h or ./myFileManager -help to show help\n");
	printf("Usage: ./myFileManager -g <file_name> <file_name2> to merge two files\n");
	printf("Usage: ./myFileManager -p <file_name> <permission> to change permission\n");
	printf("Usage: ./myFileManager -o <file_name> <user_name> group_name> to change owner and group\n");
	printf("Usage: ./myFileManager -o -u <file_name> <user_name> to change owner\n");
	printf("Usage: ./myFileManager -o -g <file_name> <group_name> to change group\n");
}

void printMenu(){
    printf("myFileManager> \n");
    printf("1. Create a file\n");
    printf("2. Delete a file\n");
    printf("3. Rename a file\n");
    printf("4. Move a file\n");
    printf("5. List files in a directory\n");
    printf("6. Show information about a file\n");
    printf("7. Merge two files\n");
    printf("8. Change permission\n");
    printf("9. Change owner and group\n");
    printf("10. Change owner\n");
    printf("11. Change group\n");
    printf("12. Exit\n");
    printf("Enter your choice: ");
}

char* getInput(const char *prompt, char *input) {
    printf(prompt);
    scanf("%s", input);
    return input;
}

int main(int argc, char *argv[])
{
    while (1)
    {
        printMenu();
        //get input and check it is a number
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > 12)
        {
            printf("Invalid input\n");
            continue;
        }

        //check if the user wants to exit
        if (choice == 12)
        {
            break;
        }

        //switch on the choice
        switch (choice)
        {
        case 1:
            // request to input the file name
            char fileName[100];
            getInput("Enter file name: ", fileName);
            createFile(fileName);
            break;
        case 2:
            // request to input the file name
            char fileName[100];
            getInput("Enter file name: ", fileName);
            deleteFile(fileName);
            break;
        case 3:
            // request to input the file name
            printf("Enter old file name: ");
            scanf("%s", oldFileName);
            printf("Enter new file name: ");
            scanf("%s", newFileName);
            renameFile(oldFileName, newFileName);
            break;
        case 4:
            // request to input the file name
            printf("Enter source file name: ");
            scanf("%s", sourceFileName);
            printf("Enter destination file name: ");
            scanf("%s", destinationFileName);
            moveFile( sourceFileName, destinationFileName);
            break;
        case 5:
            // request to input the directory path
            printf("Enter directory path: ");
            scanf("%s", directoryPath);
            listFiles( directoryPath);
            break;
        case 6:
            // request to input the file name
            printf("Enter file name: ");
            scanf("%s", fileName);
            printFileInfo(fileName);
            break;
        case 7:
            // request to input the file name
            printf("Enter file name: ");
            scanf("%s", fileName);
            printf("Enter file name2: ");
            scanf("%s", fileName2);
            mergeFiles(fileName, fileName2);
            break;
        case 8:
            // request to input the file name
            printf("Enter file name: ");
            scanf("%s", fileName);
            printf("Enter permission: ");
            scanf("%s", p);
            mode_t permission = strtol(p, NULL, 8);
            changePermission(fileName, permission);
            break;
        case 9:
            // request to input the file name
            printf("Enter file name: ");
            scanf("%s", fileName);
            printf("Enter user name: ");
            scanf("%s", user);
            printf("Enter group name: ");
            scanf("%s", group);
            changeOwnerAndGroup(fileName, user, group);
            break;
        case 10:
            // request to input the file name
            printf("Enter file name: ");
            scanf("%s", fileName);
            printf("Enter user name: ");
            scanf("%s", user);
            changeOwnerAndGroup(fileName, user, NULL);
            break;
        case 11:
            // request to input the file name
            printf("Enter file name: ");
            scanf("%s", fileName);
            printf("Enter group name: ");
            scanf("%s", group);
            changeOwnerAndGroup(fileName, NULL, group);
            break;
        default:
            printf("Invalid choice\n");
            break;
        }

    }
    
    return 0;
}
