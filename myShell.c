#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<signal.h>
#include<sys/resource.h>

#define ARRAY_SIZE     100
#define Append_To_File   "a"
/*array for handling with input*/
char input[ARRAY_SIZE]; 
/// @brief 
char * paresedInput[ARRAY_SIZE];
int counter = 0 , exportFlag = 0 , echoFlag = 0 , cdFlag = 0 , pwdFlag = 0;
int backGroundIndex = 0 , exitFlag = 0;

/*get input string from the user*/
void Get_Input(void){
    char arr[100];
    printf("%s shell >> ",getcwd(arr , 100));
    scanf("%[^\n]%*c", input);
}

/*parse the input and handle system environment commands*/

void Clean_Export(char * token){
    while(token != NULL){
            paresedInput[counter] = token;
            token = strtok(NULL,"=");
            counter++;
        }
        paresedInput[counter] = '\0';
        counter = 0;
}

void Parse_Input(void){
    char*token = strtok(input , " ");
    if(strcmp(token , "export") == 0){
        Clean_Export(token);
        exportFlag = 1;
    }
    else{
        if(strcmp(token , "cd") == 0) cdFlag = 1;
        if(strcmp(token , "echo") == 0) echoFlag = 1;
        if(strcmp(token , "pwd") == 0) pwdFlag = 1;
        if(strcmp(token , "exit") == 0) exitFlag = 1;
      while (token != NULL)
      {
          paresedInput[counter] = token;
          token = strtok(NULL," ");
          counter++;
      }   
      paresedInput[counter] = '\0' ;
      backGroundIndex = counter - 1;
      counter = 0;
    }
}

void Excute_CD(void){
    if((paresedInput[1] == NULL) || ((strcmp(paresedInput[1],"~")==0))){
        chdir(getenv("HOME"));
        
    }
    else{
         int flag = 0;
         flag = chdir(paresedInput[1]) ; 
         if( flag != 0){
            printf("Error, the directory is not found\n");
         }
    }
}

void Excute_Export(void){
    char* data = paresedInput[2];
    /*check the qutation marks*/
    if(data[0] == '"'){
        data++;
        data[strlen(data)-1] = '\0';
        setenv(paresedInput[1] , data , 1);
    }
    else{
        /*No qutation mark*/
        setenv(paresedInput[1] , paresedInput[2] , 1);
    }
}

void Excute_Echo(void){
    char*echoEnv = paresedInput[1];
    if(paresedInput[2] == NULL){
        /*there is only one command in echo*/
        /*there is two cases print variable or sentence*/

        /*remove the qoutation*/
        echoEnv++;
        echoEnv[strlen(echoEnv) - 1] = '\0';
        /*case 1  print variable*/
        if(echoEnv[0] == '$'){
            /*skip dollar sign*/
            echoEnv++;
            printf("%s\n",getenv(echoEnv));
        }
        else{
            /*case 2 print sentence*/
            printf("%s\n",echoEnv);
        }
    }
    else{
        char*temp = paresedInput[2];
        /*there is more than input*/
        /*remove the first qutation*/
        echoEnv++;
        if(echoEnv[0] == '$'){
            echoEnv++;
            printf("%s ",getenv(echoEnv));
            /*remove the last qutation*/
            temp[strlen(temp)-1] = '\0';
            printf("%s\n",temp);
        }
        else{
            printf("%s ",echoEnv);
            /*skip $*/
            temp++;
            /*remove the last qoutation*/
            temp[strlen(temp)-1] = '\0';
            printf("%s\n",getenv(temp));
        }
    }
}

void Excute_Shell_Built_In(void){
    if(cdFlag){
        Excute_CD();
    }
    else if(exportFlag){
        Excute_Export();
    }
    else if(echoFlag){
        Excute_Echo();
    }
    else if(pwdFlag){
        printf("%s\n",getcwd(NULL,0));
    }
}

void Execute_Command (void){
    int status , foregroundId;
    int errorCommand = 1;
    int child_id = fork();
    if(child_id  == -1){
        printf("System Error!\n");
        exit(EXIT_FAILURE);
    }
    else if (child_id == 0){
        if(paresedInput[1] == NULL){
            /*command consist of one word*/
            errorCommand = execvp(paresedInput[0] , paresedInput);
        }
        else if(paresedInput[1] != NULL){
            /*more than one word*/
            /*check if there is a variable in system environment or not*/
            char* env = paresedInput[1];
            if(env[0] == '$'){
                int  i = 1;
                char*envTemp;
                env++;
                envTemp = getenv(env);
                char *exportTemp = strtok(envTemp , " ");
                while(exportTemp != NULL){
                paresedInput[i++] = exportTemp;    
                exportTemp = strtok(NULL, " ");
                }
            }
            errorCommand = execvp(paresedInput[0] , paresedInput);
        }
        if(errorCommand){
            printf("Error ! unknown command\n");
            exit(EXIT_FAILURE);
        }
    }
    else{
        /*parent process*/
        /*foreground and background*/
        if(strcmp(paresedInput[backGroundIndex] , "&")==0){
            /*we are in the backGround*/
            /*no wait*/
            return;
        }
        else{
            foregroundId = waitpid(child_id , &status , 0);
            if(foregroundId == -1){
                perror("Error in waitpad function\n");
                return;
            }
        if(errorCommand){
            FILE * file = fopen("log.text" , Append_To_File);
            fprintf(file , "%s" , "Child process terminated\n");
            fclose(file);
        }
      }
    }
}

void Write_To_Log_File(void){
    FILE * file = fopen("log.text" , Append_To_File);
    if(file == NULL){
        printf("Error in file\n");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(file , "%s" , "Child process terminated\n");
        fclose(file);
    }
}


void Reap_Child_Zombie(void){
    int status;
    pid_t id = wait(&status);
    /*avoid zombie process*/
    if(id == 0 || id == -1){
        return;
    }
    else{
        Write_To_Log_File();
    }
}


void on_child_exist(){
    Reap_Child_Zombie();
}


void shell(void){
    while(1){
        Get_Input();
        Parse_Input();
          if(exitFlag){
              exitFlag = 0;
              exit(EXIT_FAILURE);
          }
          else if(cdFlag || pwdFlag || exportFlag || echoFlag){
              Excute_Shell_Built_In();
              cdFlag = 0;
              pwdFlag = 0;
              exportFlag = 0;
              echoFlag = 0;
        }
        else{
            Execute_Command();
        }
    }
}


void Setup_Environment(void){
    /* same directory of the project*/
    char arr[100];
    chdir(getcwd(arr , 100));
}


void Register_Child_Signal(void){
    signal(SIGCHLD , on_child_exist);
}

int main(){
   Register_Child_Signal();
   Setup_Environment();
   shell();
   return 0;
}