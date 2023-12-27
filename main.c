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

#define BUFFER_SIZE 1024

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

int createDirectory(const char *path) {
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        // Directory doesn't exist, create it
        if (mkdir(path, 0777) != 0) {
            // Check if the failure is due to non-existent parent directories
            char *last_slash = strrchr(path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';  // Remove the last component of the path
                if (createDirectory(path) != 0) {
                    perror("Error creating directory");
                    return -1;
                }
                *last_slash = '/';  // Restore the path
                // Retry creating the directory
                if (mkdir(path, 0777) != 0) {
                    perror("Error creating directory");
                    return -1;
                }
            } else {
                perror("Error creating directory");
                return -1;
            }
        }
    }

    return 0;  // Directory was created or already exists
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
    createDirectory(trash_path);
    strcat(trash_path, fileName);

    // Create path to trash info
    char trash_info_path[PATH_MAX + 1];
    strcpy(trash_info_path, home_path);
    strcat(trash_info_path, "/.local/share/Trash/info/");
    createDirectory(trash_info_path);
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

void deleteFile(const char *fileName) {
    // Prompt the user before deleting the file
    printf("Are you sure you want to delete the file '%s'? (y/n): ", fileName);

    char response;
    scanf(" %c", &response);  // Notice the space before %c to consume the newline character

    if (response == 'y' || response == 'Y') {
        // Delete the file
        if (remove(fileName) == 0) {
            printf("File '%s' deleted successfully.\n", fileName);
        } else {
            perror("remove");
        }
    } else {
        printf("File deletion canceled.\n");
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

void mergeFiles(const char *fileName, const char *fileName2, const char *fileName3) {
    int fd1[2];
    int fd2[2];

    FILE *fp1 = fopen(fileName, "r");
    FILE *fp2 = fopen(fileName2, "r");

    createFile(fileName3);
    FILE *fp3 = fopen(fileName3, "w");

    if (fp1 == NULL || fp2 == NULL || fp3 == NULL) {
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
    // parent process (P1)
    else if (p > 0) {
        char concat_str[60000];

        close(fd1[0]);

        // Read the entire content of file 1
        fseek(fp1, 0, SEEK_END);
        long fileSize = ftell(fp1);
        rewind(fp1);

        fread(concat_str, sizeof(char), fileSize, fp1);

        // Send the content to the child process through pipe
        write(fd1[1], concat_str, fileSize);
        close(fd1[1]);

        wait(NULL);

        close(fd2[1]);

        // Read the combined content from the child process through pipe
        read(fd2[0], concat_str, 60000);
        fprintf(fp3, "%s", concat_str);
        close(fd2[0]);
        fclose(fp3);
    }
    // child process (P2)
    else {
        close(fd1[1]);

        char concat_str[60000];
        char concat_str2[60000];

        // Read the content received from the parent through pipe
        int bytesRead = read(fd1[0], concat_str, 60000);
        printf("C1 Read %d bytes\n", bytesRead);

        // Process the content if needed

        // Read the content from file 2
        while (fgets(concat_str2, 60000, fp2) != NULL) {
            strcat(concat_str, concat_str2);
        }

        printf("C2 %s\n", concat_str);

        close(fd1[0]);
        close(fd2[0]);

        // Send the combined content to the parent through pipe
        write(fd2[1], concat_str, strlen(concat_str) + 1);
        close(fd2[1]);

        fclose(fp1);
        fclose(fp2);

        exit(0);
    }
}

void mergeFileAtLine(const char *fileMerge1, const char *fileMerge2, const char *fileFinal, int lineMerge)
{
    // open files and check if they exist
    FILE *fp1 = fopen(fileMerge1, "r");
    FILE *fp2 = fopen(fileMerge2, "r");
    if (fp1 == NULL || fp2 == NULL)
    {
        perror("fopen");
    }

    // check if lineMerge is valid
    if (lineMerge < 1)
    {
        printf("Invalid line number\n");
    }

    // create fileFinal and open it
    createFile(fileFinal);
    FILE *fp3 = fopen(fileFinal, "w");
    if (fp3 == NULL)
    {
        perror("fopen");
    }

    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
    }

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
    }

    if (pid > 0)
    {
        close(pipefd[0]);

        char line[BUFFER_SIZE];
        int lineCount = 0;
        while (fgets(line, sizeof(line), fp1) != NULL)
        {
            lineCount++;
            if (lineCount == lineMerge)
            {
                char line_2[BUFFER_SIZE];
                while (fgets(line_2, sizeof(line_2), fp2) != NULL)
                {
                    write(pipefd[1], line_2, strlen(line_2));
                }
            }
            write(pipefd[1], line, strlen(line));
        }

        close(pipefd[1]);

        wait(NULL);
    }
    else
    {
        close(pipefd[1]);

        char line[BUFFER_SIZE];

        while (read(pipefd[0], line, sizeof(line)) > 0)
        {
            fputs(line, fp3);
        }

        close(pipefd[0]);
        exit(0);
    }

    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
}

void printMenu(){
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
    printf("9. Change ownership\n");
    printf("10. Save file information\n");
    printf("11. Exit\n");
    printf("Enter your choice: ");
}

void printMenuChangeOwnership()
{
    printf("\n");
    printf("File Manager\n");
    printf("-------------\n");
    printf("1. Change owner\n");
    printf("2. Change group\n");
    printf("3. Change owner and group\n");
    printf("4. Back\n");
    printf("Enter your choice: ");
}

void printMenuMerge()
{
    printf("\n");
    printf("File Manager\n");
    printf("-------------\n");
    printf("1. Merge two files\n");
    printf("2. Merge at line\n");
    printf("3. Back\n");
    printf("Enter your choice: ");
}

void printMenuDelete(){
    printf("\n");
    printf("File Manager\n");
    printf("-------------\n");
    printf("1. Permanently delete a file\n");
    printf("2. Move a file to trash\n");
    printf("3. Back\n");
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
    char fileNameMerge1[100];
    char fileNameMerge2[100];
    char fileNameMergeFinal[100];
    char delete_at[100];
    int lineMerge;
    char line[100];

    while (1)
    {
        printMenu();
        int choice;
      
        // get input and check it is a number
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > 14)

        {
            printf("Invalid input\n");
            continue;
        }

        // check if the user wants to exit
        if (choice == 14)

        {
            break;
        }

        //switch on the choice
        switch (choice)
        {
        case 1:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            createFile(fileName);
            storeFileInfoInSharedMemory(fileName);
            break;
        case 2:
            do {
                printMenuDelete();
                int choice_delete;
                if (scanf("%d", &choice_delete) != 1 || choice_delete < 1 || choice_delete > 3)
                {
                    printf("Invalid input\n");
                    continue;
                }

                if (choice_delete == 3)
                {
                    break;
                }

                switch (choice_delete)
                {
                case 1:
                    getInput("Enter file name: ", fileName);
                    deleteFile(fileName);
                    break;
                case 2:
                    getInput("Enter file name: ", fileName);
                    moveFileToTrash(fileName);
                }


            } while (1);
            break;
        case 3:
            getInput("Enter old file name: ", oldFileName);
            getInput("Enter new file name: ", newFileName);
            renameFile(oldFileName, newFileName);
            break;
        case 4:
            getInput("Enter source file name: ", sourceFileName);
            getInput("Enter destination file name: ", destinationFileName);
            moveFile( sourceFileName, destinationFileName);
            break;
        case 5:
            getInput("Enter directory path: ", directoryPath);
            listFiles( directoryPath);
            break;
        case 6:
            getInput("Enter file name: ", fileName);
            printFileInfo(fileName);
            break;
        case 7:
            do
            {
                printMenuMerge();
                int choice_merge;
                if (scanf("%d", &choice_merge) != 1 || choice_merge < 1 || choice_merge > 2)
                {
                    printf("Invalid input\n");
                    continue;
                }

                if (choice_merge == 3)
                {
                    break;
                }

                switch (choice_merge)
                {
                case 1:
                    getInput("Enter file name 1: ", fileName);
                    while (access(fileName, F_OK) == -1)
                    {
                        printf("File '%s' does not exist.\n", fileName);
                        getInput("Enter file name 1: ", fileName);
                    }

                    getInput("Enter file name 2: ", fileName2);
                    while (access(fileName2, F_OK) == -1)
                    {
                        printf("File '%s' does not exist.\n", fileName2);
                        getInput("Enter file name 2: ", fileName2);
                    }

                    getInput("Enter file merge name: ", fileName3);
                    while (access(fileName3, F_OK) != -1)
                    {
                        printf("File '%s' already exists.\n", fileName3);
                        getInput("Enter file merge name: ", fileName3);
                    }

                    mergeFiles(fileName, fileName2, fileName3);
                    break;
                case 2:
                    getInput("Enter file name you want to merge at line: ", fileNameMerge1);
                    getInput("Enter line number: ", line);
                    lineMerge = strtol(line, NULL, 10);
                    getInput("Enter file merge name: ", fileNameMerge2);
                    getInput("Enter final file merge name: ", fileNameMergeFinal);
                    mergeFileAtLine(fileNameMerge1, fileNameMerge2, fileNameMergeFinal, lineMerge);
                    break;
                default:
                    printf("Invalid choice\n");
                    break;
                }
            } while (1);

            break;
        case 8:
            getInput("Enter file name: ", fileName);
            getInput("Enter permission: ", p);
            mode_t permission = strtol(p, NULL, 8);
            changePermission(fileName, permission);
            break;
        case 9:
             do
            {
                printMenuChangeOwnership();
                int choice_ownership;
                if (scanf("%d", &choice_ownership) != 1 || choice_ownership < 1 || choice_ownership > 4)
                {
                    printf("Invalid input\n");
                    continue;
                }
                if (choice_ownership == 4)
                {
                    break;
                }
                switch (choice_ownership)
                {
                case 1:
                    getInput("Enter file name: ", fileName);
                    getInput("Enter user name: ", user);
                    changeOwnerAndGroup(fileName, user, NULL);
                    break;
                case 2:
                    getInput("Enter file name: ", fileName);
                    getInput("Enter group name: ", group);
                    changeOwnerAndGroup(fileName, NULL, group);
                    break;
                case 3:
                    getInput("Enter file name: ", fileName);
                    getInput("Enter user name: ", user);
                    getInput("Enter group name: ", group);
                    changeOwnerAndGroup(fileName, user, group);
                    break;
                default:
                    printf("Invalid choice\n");
                    break;
                }
            } while (1);
            break;
        case 10:
            // request to store file information in shared memory
            getInput("Enter file name: ", fileName);
            storeFileInfoInSharedMemory(fileName);
            break;
        case 11:
            exit(EXIT_SUCCESS);
            break;
        default:
            printf("Invalid choice\n");
            break;
        }

    }
    
    return 0;
}
