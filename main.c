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
#include <stdarg.h>


#define MAX_STACK_SIZE 100


typedef struct {
    int choice;
    char fileName[100];
    char oldFileName[100];
    char newFileName[100]; 
    char sourceFileName[100];
    char destinationFileName[100];
    char permission[100];
    char user[100];
    char group[100];
    // Các trạng thái khác cần lưu trữ tùy thuộc vào lệnh
} CommandState;

typedef struct {
    CommandState states[MAX_STACK_SIZE];
    int top;
} UndoStack;

// Khởi tạo ngăn xếp
void initializeStack(UndoStack *stack) {
    stack->top = -1;
}

// Thêm một trạng thái vào ngăn xếp
void push(UndoStack *stack, CommandState state) {
    if (stack->top < MAX_STACK_SIZE - 1) {
        stack->top++;
        stack->states[stack->top] = state;
    } else {
        printf("Ngăn xếp đầy. Không thể thêm trạng thái mới.\n");
    }
}

// Lấy trạng thái cuối cùng ra khỏi ngăn xếp
CommandState pop(UndoStack *stack) {
    CommandState emptyState = {0};  // Trạng thái rỗng
    if (stack->top >= 0) {
        CommandState state = stack->states[stack->top];
        stack->top--;
        return state;
    } else {
        printf("Ngăn xếp rỗng. Không thể hoàn tác nữa.\n");
        return emptyState;
    }
}


void history(const char *format, ...) {
    FILE *historyFile = fopen("history.txt", "a");

    if (historyFile != NULL) {
        time_t currentTime;
        time(&currentTime);
        struct tm *localTime = localtime(&currentTime);

        va_list args;
        va_start(args, format);

        fprintf(historyFile, "[%04d-%02d-%02d %02d:%02d:%02d] ", 
                localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
                localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

        vfprintf(historyFile, format, args);
        fprintf(historyFile, "\n");

        va_end(args);

        fclose(historyFile);
    } else {
        perror("fopen");
    }
}

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
    history("saved information of file '%s'.", fileInfo.fileName);
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
    history("saw information of file '%s'.", fileInfo.fileName);
}

void createFolder(const char *folderName)
{
    // create a folder
    if (mkdir(folderName, 0777) == 0)
    {
        printf("Folder '%s' created successfully.\n", folderName);
	history("created folder '%s'.", folderName);
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
	history("created file '%s'.", fileName);
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
    history("moved file '%s' to trash.", fileName);
}


void restoreFileFromTrash(const char *fileName)
{
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

    // Check if the file and its corresponding info file in trash exist
    if (access(trash_path, F_OK) == 0 && access(trash_info_path, F_OK) == 0)
    {
        // Open trash info file
        FILE *trash_info_file = fopen(trash_info_path, "r");
        if (trash_info_file == NULL)
        {
            perror("fopen trash info");
            return;
        }

        // Read information from the trash info file
        char path_from_info[PATH_MAX + 1];
        char deletion_date[50];
        if (fscanf(trash_info_file, "[Trash Info]\nPath=%s\nDeletionDate=%s", path_from_info, deletion_date) != 2)
        {
            fprintf(stderr, "Error reading trash info file.\n");
            fclose(trash_info_file);
            return;
        }

        // Close the trash info file
        fclose(trash_info_file);

        // Move the file back to its original location
        if (rename(trash_path, path_from_info) == 0)
        {
            printf("Restored file '%s' from trash.\n", fileName);
	    history("restored file '%s' from trash.", fileName);
        }
        else
        {
            perror("rename");
        }
    }
    else
    {
        printf("File '%s' not found in trash.\n", fileName);
    }
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
	    history("deleted file '%s'.", fileName);
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
	history("renamed file '%s' to '%s'.", oldName, newName);
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
	history("moved file '%s' to '%s'.", source, destination);
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
	history("opened list files");
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
	history("changed permission of file '%s' to '%s'.", fileName, permission);
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
	history("changed owner and group of file '%s' to '%s:%s'.", file_path, user_name, group_name);
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
	history("merged file '%s' and '%s' to '%s'.", fileName, fileName2, fileName3);
}

void duplicateFile(const char *fileName) {
    FILE *fp = fopen(fileName, "r");
    if (fp == NULL) {
        perror("fopen");
        return;
    }

    char dupliFile[50];
    strcpy(dupliFile, "copy_");
    strcat(dupliFile, fileName);

    createFile(dupliFile);
    FILE *dupliFp = fopen(dupliFile, "w");
    if (dupliFp == NULL) {
        perror("createFile");
        fclose(fp);
        return;
    }
    int character;
    while ((character = fgetc(fp)) != EOF) {
        fputc(character, dupliFp);
    }

    fclose(fp);
    fclose(dupliFp);

    printf("File '%s' duplicated successfully.\n", fileName);
    history("duplicated file '%s'.", fileName);
}

void openHistory()
{
   const char *history = "history.txt";

    FILE *file = fopen(history, "r");

    if (file == NULL) {
        printf("can't open file %s\n", history);
    }

    // Đọc nội dung từ file
    printf("history of system:\n");
    int kyTu;
    while ((kyTu = fgetc(file)) != EOF) {
        putchar(kyTu);

    }

    fclose(file);
}

void undoMoveFile(const char *sourceFileName, const char *destinationFileName) {
    // Di chuyển file trở lại vị trí ban đầu
    if (rename(destinationFileName, sourceFileName) == 0) {
        printf("Undo successful. File '%s' moved back to '%s'.\n", destinationFileName, sourceFileName);
    } else {
        perror("rename");
    }
}

void undoChangePermission(const char *fileName, const char *permission) {
	mode_t oPermission = (mode_t)strtol(permission, NULL, 8);
    if (chmod(fileName, oPermission) == 0) {
        printf("Undo successful. Permission of '%s' restored to %o.\n", fileName, oPermission);
    } else {
        perror("chmod");
    }
}

void undoChangeOwnerAndGroup(const char *fileName, const char *user, const char *group) {
    struct passwd *pwd = getpwnam(user);
    struct group *grp = getgrnam(group);

    if (chown(fileName, pwd->pw_uid, grp->gr_gid) == 0) {
        printf("Undo successful. File '%s' rechange owner and group: '%s:%s'.\n", fileName, user, group);
    } else {
        perror("rename");
    }
}

// Hàm undo
void undo(UndoStack *stack) {
    if (stack->top >= 0) {
        CommandState prevState = pop(stack);

        // Thực hiện hoàn tác dựa trên trạng thái trước đó (prevState)
        switch (prevState.choice) {
            case 1:
                // Nếu trạng thái trước đó là việc tạo file, thì xóa file đã tạo
                if (remove(prevState.fileName) == 0) {
                    printf("Undo successful. File '%s' deleted.\n", prevState.fileName);
                } else {
                    perror("remove");
                }
                break;
	     case 2:
                // Nếu trạng thái trước đó là việc di chuyển file vào thùng rác, thì di chuyển lại file
                restoreFileFromTrash(prevState.fileName);
		printf("Undo successful. File '%s' restored.\n", prevState.fileName);
     
                break;
	     case 4: 
                if (rename(prevState.newFileName, prevState.oldFileName) == 0) {
                    printf("Undo successful. File '%s' renamed back to '%s'.\n", prevState.newFileName, prevState.oldFileName);
            	} else {
                    perror("rename");
            	}
            	break;
	     case 5:
		undoMoveFile(prevState.sourceFileName, prevState.destinationFileName);
            	break;
	     case 9:
		undoChangePermission(prevState.fileName, prevState.permission);
            	break;
	     case 10:
		undoChangeOwnerAndGroup(prevState.fileName, prevState.user, prevState.group);
            	break;

            // Các trường hợp khác nếu cần thêm vào
            // ...
            default:
                printf("Unsupported undo operation.\n");
                break;
        }
    } else {
        printf("Không có thao tác để hoàn tác.\n");
    }
}



void printMenu(){
    printf("\n");
    printf("File Manager\n");
    printf("-------------\n");
    printf("1. Create a file\n");
    printf("2. Move a file to trash\n");
    printf("3. Permanently delete a file\n");
    printf("4. Rename a file\n");
    printf("5. Move a file\n");
    printf("6. List files in a directory\n");
    printf("7. Show information about a file\n");
    printf("8. Merge two files\n");
    printf("9. Change permission\n");
    printf("10. Change owner and group\n");
    printf("11. Change owner\n");
    printf("12. Change group\n");
    printf("13. Save file information\n");
    printf("14. Duplicate file\n");
    printf("15. History\n");
    printf("16. Undo\n");
    printf("17. Exit\n");
    printf("Enter your choice: ");
}

char* getInput(const char *prompt, char *input) {
    printf("%s", prompt);
    scanf("%s", input);
    return input;
}

int main(int argc, char *argv[])
{

    UndoStack undoStack;
    initializeStack(&undoStack);

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
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > 17)

        {
            printf("Invalid input\n");
            continue;
        }

        // check if the user wants to exit
        if (choice == 17)

        {
            break;
        }

	CommandState currentState;
        //switch on the choice
        switch (choice)
        {
        case 1:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            createFile(fileName);
            storeFileInfoInSharedMemory(fileName);

	    
	    currentState.choice = 1;
	    strcpy(currentState.fileName, fileName);
	    push(&undoStack, currentState);

            break;
        case 2:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            moveFileToTrash(fileName);

	    
	    currentState.choice = 2;
	    strcpy(currentState.fileName, fileName);
	    push(&undoStack, currentState);

            break;
        case 3:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            deleteFile(fileName);

            break;
        case 4:
            // request to input the file name
            getInput("Enter old file name: ", oldFileName);
            getInput("Enter new file name: ", newFileName);
            renameFile(oldFileName, newFileName);


	    currentState.choice = 4;
	    strcpy(currentState.oldFileName, oldFileName);
	    strcpy(currentState.newFileName, newFileName);
	    push(&undoStack, currentState);
            break;
        case 5:
            // request to input the file name
            getInput("Enter source file name: ", sourceFileName);
            getInput("Enter destination file name: ", destinationFileName);
            moveFile( sourceFileName, destinationFileName);

	     currentState.choice = 5;
    	     strcpy(currentState.sourceFileName, sourceFileName);
    	     strcpy(currentState.destinationFileName, destinationFileName);
   	     push(&undoStack, currentState);
            break;
        case 6:
            // request to input the directory path
            getInput("Enter directory path: ", directoryPath);
            listFiles( directoryPath);
            break;
        case 7:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            printFileInfo(fileName);
            break;
        case 8:
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
	    currentState.choice = 1;
	    strcpy(currentState.fileName, fileName3);
	    push(&undoStack, currentState);
            break;

        case 9:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            getInput("Enter permission: ", p);
            mode_t permission = strtol(p, NULL, 8);
            changePermission(fileName, permission);

	    currentState.choice = 9;
	    strcpy(currentState.fileName, fileName);
	    sprintf(currentState.permission, "%o", permission);
	    push(&undoStack, currentState);
            break;
        case 10:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            getInput("Enter user name: ", user);
            getInput("Enter group name: ", group);
            changeOwnerAndGroup(fileName, user, group);

	    currentState.choice = 10;
	    strcpy(currentState.fileName, fileName);
	    strcpy(currentState.user, user);
	    strcpy(currentState.group, group);
	    push(&undoStack, currentState);
            break;
        case 11:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            getInput("Enter user name: ", user);
            changeOwnerAndGroup(fileName, user, NULL);
            break;
        case 12:
            // request to input the file name
            getInput("Enter file name: ", fileName);
            getInput("Enter group name: ", group);
            changeOwnerAndGroup(fileName, NULL, group);
            break;
        case 13:
            // request to store file information in shared memory
            getInput("Enter file name: ", fileName);
            storeFileInfoInSharedMemory(fileName);
            break;
	case 14:
            // request to duplicate file
            getInput("Enter file name: ", fileName);
            duplicateFile(fileName);

	    currentState.choice = 1;
	    strcpy(currentState.fileName, "copy_");
    	    strcat(currentState.fileName, fileName);
	    push(&undoStack, currentState);
            break;
	case 15:
            // request to open history
	    openHistory();
            break;
	case 16:
            // request to open history
	    undo(&undoStack);
            break;
        default:
            printf("Invalid choice\n");
            break;
        }

    }
    
    return 0;
}
