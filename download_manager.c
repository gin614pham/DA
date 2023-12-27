#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Function to download a link and show progress
void downloadLink(const char *link) {
    // Get the user's home directory
    const char *homeDirectory = getenv("HOME");

    if (homeDirectory == NULL) {
        fprintf(stderr, "Error: Unable to determine the home directory.\n");
        return;
    }

    // Specify the download directory as the user's home directory + Downloads
    const char *downloadDirectory = "/Downloads/";

    // Build the full download path
    char fullPath[256];
    snprintf(fullPath, sizeof(fullPath), "%s%s", homeDirectory, downloadDirectory);

    // Build the download command
    char command[512];
    snprintf(command, sizeof(command), "wget --progress=bar:force -P %s \"%s\" 2>&1", fullPath, link);

    // Open a pipe to capture the output of the command
    FILE *pipe = popen(command, "r");
    if (pipe == NULL) {
        perror("Error opening pipe");
        return;
    }

    // Read the output of the command and display it in real-time
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        printf("%s", buffer);
        fflush(stdout); // Ensure that the output is immediately printed
    }

    // Close the pipe
    if (pclose(pipe) == -1) {
        perror("Error closing pipe");
    }

    // Implement additional logic if needed
    printf("Download of %s complete.\n", link);
}

// Function to remove a link from the queue
void removeLink(char *downloadQueue[], int *queueSize, int index) {
    if (index >= 0 && index < *queueSize) {
        // Free memory for the link at index
        free(downloadQueue[index]);

        // Shift elements to fill the gap
        for (int i = index; i < *queueSize - 1; i++) {
            downloadQueue[i] = downloadQueue[i + 1];
        }

        // Decrement the queue size
        (*queueSize)--;
    } else {
        printf("Invalid index for removal.\n");
    }
}


// Function to show the download queue
void showQueue(char *downloadQueue[], int queueSize) {
    printf("Download Queue:\n");
    for (int i = 0; i < queueSize; i++) {
        printf("%d. %s\n", i + 1, downloadQueue[i]);
    }
}

// Function to handle case 2 (Show download queue and remove link)
void handleShowQueue(char *downloadQueue[], int *queueSize) {
    showQueue(downloadQueue, *queueSize);

    if (*queueSize > 0) {
        printf("Options after showing the queue:\n");
        printf("1. Remove link\n");
        printf("2. Return\n");
        printf("Enter your choice: ");

        int subChoice;
        scanf("%d", &subChoice);

        switch (subChoice) {
            case 1:
                // Remove link
                printf("Enter the index to remove from the queue: ");
                int index;
                scanf("%d", &index);

                removeLink(downloadQueue, queueSize, index - 1);
                break;
            case 2:
                // Return without removing link
                break;
            default:
                printf("Invalid choice after showing the queue.\n");
                break;
        }
    }
}

// Function to start the download queue
void startQueue(char *downloadQueue[], int *queueSize) {
    printf("Starting the queue...\n");

    while (*queueSize > 0) {
        downloadLink(downloadQueue[0]);
        removeLink(downloadQueue, queueSize, 0);
    }

    printf("Queue is empty.\n");

    // Free memory for each link in the queue
    for (int i = 0; i < *queueSize; i++) {
        free(downloadQueue[i]);
    }
}

// Function to show download progress
void showDownloadProgress(char *downloadQueue[], int queueSize) {
    if (queueSize > 0) {
        printf("Download Progress:\n");
        for (int i = 0; i < queueSize; i++) {
            printf("%d. %s: In progress\n", i + 1, downloadQueue[i]);
        }
    } else {
        printf("No downloads in progress.\n");
    }
}

int main() {
    char *downloadQueue[100];  // Assuming a maximum of 100 links in the queue
    int queueSize = 0;

    while (1) {
        printf("\nOptions:\n");
        printf("1. Add link to download queue\n");
        printf("2. Show download queue\n");
        printf("3. Start the queue\n");
        printf("4. Quit\n");
        printf("Enter your choice: ");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1: {
                // Add link to download queue
                char link[256];
                printf("Enter the link to add to the queue: ");
                scanf("%s", link);

                // Allocate memory for the link and add it to the queue
                downloadQueue[queueSize] = strdup(link);
                queueSize++;

                break;
            }
            case 2:
                // Handle case 2
                handleShowQueue(downloadQueue, &queueSize);
                break;
            case 3:
                // Start the queue
                startQueue(downloadQueue, &queueSize);
                break;
            case 4:
                // Quit
                printf("Quitting...\n");
                // Free memory for each link in the queue
                for (int i = 0; i < queueSize; i++) {
                    free(downloadQueue[i]);
                }

                return 0;
            default:
                printf("Invalid choice. Please enter a valid option.\n");
                break;
        }
    }

    return 0;
}
