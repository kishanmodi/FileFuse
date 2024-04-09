#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_FILE_COUNT 1000
#define MAX_FILE_NAME_LENGTH 512

char** fileNames = NULL;
char **requiredFiles = NULL;
int fileCount = 0;
int extentionFileCount = 0;

//function declaration
int list_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

int string_ends_with(const char *s, const char *suffix);

void move_file(const char *old_path, const char *new_path);

void copy_file(const char *old_path, const char *new_path);

char *get_full_pathname(const char *dirPath, const char *filename);

const char *get_file_name(const char *filePath);

int check_directory(const char *dirPath);

void exit_the_program(int status);

//main
int main(int argc, char *argv[]) {

    char *rootDir = *(argv + 1);

    int searchSuccessful = 0;

    //checking the root directory
    if(check_directory(rootDir))
    {
        //iterating to the files present in the root dir
        if (nftw(rootDir, list_files, 20, FTW_PHYS) == -1) 
        {
            perror("Error Reading the directory");

            exit_the_program(1);
        }

        //finding the file
        if(argc == 3)
        {
            const char *filename = *(argv + 2);

            //iterating and finding the file
            for (int i = 0; i < fileCount; i++) 
            {
                if(strcmp(get_file_name(*(fileNames + i)), filename) == 0)
                {
                    printf("%s\n", *(fileNames+i));

                    searchSuccessful = 1;
                }
            }
            
            //condition if the search doesn't have the file
            if(searchSuccessful == 0)
            {
                printf("Search Unsuccessful\n");
            }
        }

        //move or copy file
        if(argc == 5)
        {
            char *storageDir = *(argv + 2);

            char *option = *(argv + 3);

            char *filename = *(argv + 4);

            //checking storage dir
            if(check_directory(storageDir))
            {
                for (int i = 0; i < fileCount; i++) 
                {
                    //move operation
                    if(strcmp(option,"-mv") == 0)
                    {
                        if(strcmp(get_file_name(*(fileNames + i)), filename) == 0)
                        {
                            printf("%s\n\n",*(fileNames + i));

                            move_file(*(fileNames + i),get_full_pathname(storageDir,filename));

                            printf("Search Successful\n");

                            printf("File moved to the storage_dir\n");

                            exit_the_program(0);
                        }
                    }

                    //copy operation
                    else if(strcmp(option,"-cp") == 0)
                    {
                        if(strcmp(get_file_name(*(fileNames + i)), filename) == 0)
                        {
                            copy_file(*(fileNames + i),get_full_pathname(storageDir,filename));

                            printf("Search Successful\n");

                            printf("File copied to the storage_dir\n");

                            exit_the_program(0);
                        }
                    }
                    else
                    {
                        //error if the option is wrong
                        printf("Invalid option: Pass either -mv or -cp\n");
                    }
                }

                printf("Search Unsuccessful\n");
            }
            else
            {
                printf("Search Successful : Invalid storage_dir\n");
            }
        }

        //copy all the files from dir to storage by converting them into tar
        if(argc == 4)
        {
            char *storageDir = *(argv + 2);

            char *extension = *(argv + 3);

            if(*(extension) == '.')
            {
                for (int i = 0; i < fileCount; i++) 
                {
                    if(string_ends_with(*(fileNames + i), extension))
                    {
                        char *fileMatch = malloc(MAX_FILE_NAME_LENGTH);

                        if (fileMatch == NULL)
                        {
                            perror("malloc");

                            exit_the_program(1);
                        }

                        strcpy(fileMatch,*(fileNames + i));

                        char **temp = realloc(requiredFiles, (extentionFileCount + 1) * sizeof(char*));

                        if (temp == NULL) 
                        {
                            perror("realloc");
                            
                            exit_the_program(1);
                        }

                        requiredFiles = temp;

                        *(requiredFiles + extentionFileCount) = fileMatch;

                        extentionFileCount++;
                    }
                }
                
                //checking a temp location to put the files
                if(!check_directory("./a1"))
                {
                    //creating a temp location
                    if (mkdir("./a1", 0777) == -1)
                    {
                        perror("folder creation error");

                        exit_the_program(1);
                    }
                }

                //copying files with required extensions to the temporary folder
                for (int i = 0; i < extentionFileCount; i++)
                {
                    copy_file(*(requiredFiles + i), get_full_pathname("./a1",get_file_name(*(requiredFiles + i))));
                }

                //creating tar
                if(system("tar -cf ./a1.tar a1/*")==0)
                {
                    if(!check_directory(storageDir))
                    {
                        //creating a temp location
                        if (mkdir(storageDir, 0777) == -1)
                        {
                            perror("folder creation error");

                            exit_the_program(1);
                        }
                    }

                    //moving the tar to the storage dir
                    move_file("./a1.tar",get_full_pathname(storageDir,"a1.tar"));
                    
                    //removing the temp folder
                    system("rm -rf ./a1");
                }
                else
                {
                    printf("Error creating the tar file\n");
                }
            }
            else
            {
                printf("Please check the arguments\n");
            }
        }
    }
    else
    {
        printf("invalid root_dir\n");
    }

    exit_the_program(0);
}

//checking the directory
//0: directory not present
//1: directory present
int check_directory(const char *dirPath)
{
    if(opendir(dirPath) == NULL)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//concatenate the dirpath and filename
char *get_full_pathname(const char *dirPath, const char *filename)
{
    char *temp = malloc(MAX_FILE_NAME_LENGTH);

    strcpy(temp,dirPath);

    if(string_ends_with(temp,"/") == 0)
    {
        strcat(temp,"/");
    }

    strcat(temp,filename);

    return temp;
}

//moving the file from oldPath to newPath
void move_file(const char *oldPath, const char *newPath)
{
    if (rename(oldPath, newPath) == 0) 
    {
        printf("File moved successfully.\n");
    } 
    else 
    {
        perror("Error moving file");
        exit_the_program(1);
    }
}

//copying the file from oldPath to newPath by creating a new file
void copy_file(const char *oldPath, const char *newPath)
{
    int fd1 = open(oldPath, O_RDONLY);

    if (fd1 == -1) 
    {
        perror("Error opening source file");

        exit_the_program(1);
    }

    //Checking the size of the file
    off_t file_size = lseek(fd1, 0, SEEK_END);

    lseek(fd1, 0, SEEK_SET);

    // taking the same size in memory
    char *file_content = malloc(file_size + 1);

    if (file_content == NULL) 
    {
        perror("Memory allocation failed");

        close(fd1);

        exit_the_program(1);
    }

    //reading the file
    ssize_t bytes_read = read(fd1, file_content, file_size);

    if (bytes_read == -1)
     {
        perror("Error reading from source file");

        close(fd1);

        free(file_content);

        exit_the_program(1);
    }

    *(file_content + bytes_read) = '\0';

    close(fd1);

    //making the file
    int fd2 = open(newPath, O_WRONLY | O_CREAT | O_TRUNC, 0700);

    if (fd2 == -1) 
    {
        perror("Error opening destination file");

        free(file_content);

        exit_the_program(1);
    }

    // writing the content in the file
    ssize_t bytes_written = write(fd2, file_content, bytes_read);

    if (bytes_written == -1) 
    {
        perror("Error writing to destination file");

        close(fd2);

        free(file_content);

        exit_the_program(1);
    }

    close(fd2);

    free(file_content);
}

//checks the string whether ends with substring or not
int string_ends_with(const char *string, const char *substring)
{
    int string_length = strlen(string);

    int substring_length = strlen(substring);

    return substring_length <= string_length && !strcmp(string + string_length - substring_length, substring);
}

//getting just the file name from the absolute path
const char *get_file_name(const char *filePath)
{
    const char* fileName = strrchr(filePath, '/');

    if (fileName != NULL) 
    {
        fileName++;
    } 
    else 
    {
        fileName = filePath;
    }

    return fileName;
}

//function that iterates to the root dir and store all the present file available
int list_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) 
{
    if (typeflag == FTW_F) 
    {
        char *filename = malloc(MAX_FILE_NAME_LENGTH);

        if (filename == NULL)
        {
            perror("malloc");

            exit_the_program(1);
        }

        strncpy(filename, fpath, MAX_FILE_NAME_LENGTH - 1);

        //sizeof(char*) b'coz it is a pointer to that the string address
        char **temp = realloc(fileNames, (fileCount + 1) * sizeof(char*));
        
        if (temp == NULL) 
        {
            perror("realloc");

            exit_the_program(1);
        }

        fileNames = temp;

        *(fileNames + fileCount) = filename;

        fileCount++;
    }

    return 0;
}

//function that is called on failure condition
//it clears the pointers and exit the program
void exit_the_program(int status)
{
    if (fileNames != NULL) 
    {
        for (int i = 0; i < fileCount; i++) 
        {
            free(fileNames[i]);
        }
        
        free(fileNames);
        
        fileNames = NULL;
    }

    if (requiredFiles != NULL) 
    {
        for (int i = 0; i < extentionFileCount; i++) 
        {
            free(requiredFiles[i]);
        }
        
        free(requiredFiles);
        
        requiredFiles = NULL;
    }

    status == 0 ? exit(EXIT_SUCCESS) : exit(EXIT_FAILURE);
}

