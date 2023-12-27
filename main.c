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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <linux/limits.h>

void storeFileInfoInSharedMemory(const char *filePath);
struct FileInfo
{
    char fileName[100];
    mode_t permission;
    uid_t uid;
    gid_t gid;
    off_t fileSize;
    time_t lastAccessTime;
};

// Function to store file information in shared memory
void storeFileInfoInSharedMemory(const char *filePath)
{
    struct FileInfo fileInfo;

    // Get file information
    struct stat fileStat;
    if (stat(filePath, &fileStat) < 0)
    {
        perror("stat");
        return;
    }

    // Store file information in struct FileInfo
    strcpy(fileInfo.fileName, filePath);
    fileInfo.permission = fileStat.st_mode & 0777;
    fileInfo.uid = fileStat.st_uid;
    fileInfo.gid = fileStat.st_gid;
    fileInfo.fileSize = fileStat.st_size;
    fileInfo.lastAccessTime = fileStat.st_atime;

    // Create shared memory segment
    key_t key = ftok(filePath, 'R');
    int shmid = shmget(key, sizeof(struct FileInfo), IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        return;
    }

    // Attach shared memory segment
    struct FileInfo *sharedFileInfo = (struct FileInfo *)shmat(shmid, NULL, 0);
    if (sharedFileInfo == (struct FileInfo *)-1)
    {
        perror("shmat");
        return;
    }

    // Copy file information to shared memory
    memcpy(sharedFileInfo, &fileInfo, sizeof(struct FileInfo));

    // Detach shared memory segment
    shmdt(sharedFileInfo);
}

// Function to print file information
void printFileInfo(const char *filePath)
{
    struct FileInfo fileInfo;

    // Get shared memory segment
    key_t key = ftok(filePath, 'R');
    int shmid = shmget(key, sizeof(struct FileInfo), 0666);
    if (shmid < 0)
    {
        perror("shmget");
        return;
    }

    // Attach shared memory segment
    struct FileInfo *sharedFileInfo = (struct FileInfo *)shmat(shmid, NULL, 0);
    if (sharedFileInfo == (struct FileInfo *)-1)
    {
        perror("shmat");
        return;
    }

    // Copy file information from shared memory
    memcpy(&fileInfo, sharedFileInfo, sizeof(struct FileInfo));

    // Detach shared memory segment
    shmdt(sharedFileInfo);

    // Print file information
    printf("Information for '%s':\n", fileInfo.fileName);
    printf("Permission: %o\n", fileInfo.permission);
    printf("Owner UID: %d\n", fileInfo.uid);
    printf("Group GID: %d\n", fileInfo.gid);
    printf("File Size: %ld bytes\n", (long)fileInfo.fileSize);
    printf("Last Access Time: %s", ctime(&(fileInfo.lastAccessTime)));
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

void mergeFiles(const char *fileName, const char *fileName2, const char *fileName3)
{
    int fd1[2];
    int fd2[2];

    FILE *fp1 = fopen(fileName, "r");
    FILE *fp2 = fopen(fileName2, "r");

    createFile(fileName3);
    FILE *fp3 = fopen(fileName3, "w");

    if (fp1 == NULL || fp2 == NULL)
    {
        perror("fopen");
        return;
    }

    const int BUFFER_SIZE = 1024;
    char concat_str[BUFFER_SIZE];
    char concat_str2[BUFFER_SIZE];

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
        close(fd1[0]);

        while (fgets(concat_str, sizeof(concat_str), fp1) != NULL)
        {
            write(fd1[1], concat_str, strlen(concat_str) + 1);
        }

        close(fd1[1]);

        wait(NULL);

        close(fd2[1]);

        while (read(fd2[0], concat_str, sizeof(concat_str)) > 0)
        {
            fputs(concat_str, fp3);
        }

        close(fd2[0]);
        fclose(fp3);
    }
    // child process
    else
    {
        close(fd1[1]);

        while (read(fd1[0], concat_str, sizeof(concat_str)) > 0)
        {
            // Read from fp2 and concatenate
            while (fgets(concat_str2, sizeof(concat_str2), fp2) != NULL)
            {
                strcat(concat_str, concat_str2);
            }

            // Write to fd2[1]
            write(fd2[1], concat_str, strlen(concat_str) + 1);
        }

        close(fd1[0]);
        close(fd2[0]);
        close(fd2[1]);

        exit(0);
    }
}

void printMenu()
{
    printf("\n");
    printf("File Manager\n");
    printf("-------------\n");
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
    printf("12. Save file information\n");
    printf("13. Exit\n");
    printf("Enter your choice: ");
}

char *getInput(const char *prompt, char *input)
{
    printf("%s", prompt);
    scanf("%s", input);
    return input;
}

int main(int argc, char *argv[])
{
    char fileName[100];
    char oldFileName[100];
    char newFileName[100];
    char sourceFileName[100];
    char destinationFileName[100];
    char user_name[100];
    char group_name[100];
    char fileName2[100];
    char fileName3[100];
    char p[100];
    char user[100];
    char group[100];
    char directory[100];
    char directoryPath[100];

    while (1)
    {
        printMenu();
        int choice;
        // get input and check it is a number
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > 13)
        {
            printf("Invalid input\n");
            continue;
        }

        // check if the user wants to exit
        if (choice == 13)
        {
            break;
        }

        // switch on the choice
        switch (choice)
        {
        case 1:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            createFile(fileName);
            storeFileInfoInSharedMemory(fileName);
            break;
        case 2:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            deleteFile(fileName);
            break;
        case 3:
            // request to input the file name
            getInput("Enter old file name: ", oldFileName);
            getInput("Enter new file name: ", newFileName);
            renameFile(oldFileName, newFileName);
            break;
        case 4:
            // request to input the file name
            getInput("Enter source file name: ", sourceFileName);
            getInput("Enter destination file name: ", destinationFileName);
            moveFile(sourceFileName, destinationFileName);
            break;
        case 5:
            // request to input the directory path
            getInput("Enter directory path: ", directoryPath);
            listFiles(directoryPath);
            break;
        case 6:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            printFileInfo(fileName);
            break;
        case 7:
            // request to input the file names
            getInput("Enter file name 1: ", fileName);
            while (access(fileName, F_OK) == -1)
            {
                printf("File '%s' does not exist.\n", fileName);
                // Prompt the user again for file 1
                getInput("Enter file name 1: ", fileName);
            }

            getInput("Enter file name 2: ", fileName2);
            while (access(fileName2, F_OK) == -1)
            {
                printf("File '%s' does not exist.\n", fileName2);
                // Prompt the user again for file 2
                getInput("Enter file name 2: ", fileName2);
            }

            getInput("Enter file merge name: ", fileName3);
            while (access(fileName3, F_OK) != -1)
            {
                printf("File '%s' already exists.\n", fileName3);
                // Prompt the user again for file merge name
                getInput("Enter file merge name: ", fileName3);
            }

            // Valid file names, merge files
            mergeFiles(fileName, fileName2, fileName3);
            break;
        case 8:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            getInput("Enter permission: ", p);
            mode_t permission = strtol(p, NULL, 8);
            changePermission(fileName, permission);
            break;
        case 9:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            getInput("Enter user name: ", user);
            getInput("Enter group name: ", group);
            changeOwnerAndGroup(fileName, user, group);
            break;
        case 10:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            getInput("Enter user name: ", user);
            changeOwnerAndGroup(fileName, user, NULL);
            break;
        case 11:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            getInput("Enter group name: ", group);
            changeOwnerAndGroup(fileName, NULL, group);
            break;
        case 12:
            // request to store file information in shared memory
            getInput("Enter file name: ", fileName);
            storeFileInfoInSharedMemory(fileName);
            break;
        default:
            printf("Invalid choice\n");
            break;
        }
    }

    return 0;
}
