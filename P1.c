#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

int H;
int L;
char *numHiddenKeys = "10";
int PN = 4;//THIS SHOULD BE INCREASED FOR BETTER ALGORITHM PERFORMANCE
int fd[2];
int cd[2];
int factor = 100000; //Factor to bind the random numbers by, increase or decrease to change random number range 0 < num < factor + 1
int len;
FILE *fp;
//FUNCTION DECLARATIONS
int getUserInput(char *req);
void emptyScan();
int addLines(FILE *tfp);
int createChildProcesses(int pn, pid_t *pid);
int childProcessCode(int index);
int *childProcessMethod(int *array, int count);
int *fileArray;//Will be the addresses to integers
//------------------------------------------

int main(int argc, char *argv[]){
	pid_t pid[PN];
	//Set the seed for the random number
	srand(time(0));
	//First thing needed is to create a file to put all of the lines
	fp = fopen("RandomNumFile", "wb"); //Create file or open existing file "RandomNumFile", truncate to length 0 for writing
	if(fp == NULL){
		//Something wrong happened
		printf("Could not open or create file");
		exit(10);
	}
	//Here we are going to get L from the user
	L = getUserInput("L");
	//Initialize an array of integers, representation of our file
	int tfileArray[L];
	fileArray = tfileArray;
	int q = addLines(fp);
	printf("Length of file is %d bytes\n", len);
	//Here we are going to get H from the user
	H = getUserInput("H");
	q = createChildProcesses(PN, pid);
	int offset = 0;
	int rec;
	//This allows the child processes to read the file and process the information
	for(int w=0; w < PN; w++){
		int package[4] = {L, PN, len, offset};
		write(fd[1],package,4*sizeof(int));
		wait(NULL);
		read(cd[0], &rec, sizeof(int));
		offset = offset + rec;
	}
	printf("Parent process finished\n");
}


//Returns the number of hidden keys found
int childProcessCode(int index){
	int package[4];
	int n = read(fd[0], &package, 4*sizeof(int));
	int linestoRead = package[0]/package[1] + 1;
	int len = package[2];
	int offset = package[3];
	int count = 0;
	int array[linestoRead];
	int numindex = 0;
	printf("child process %d read data from buffer and will attempt to read %d lines, of the total %d bytes starting from %d\n", getpid(), linestoRead, len, offset);
	FILE *nfp = fopen("RandomNumFile","r");
	fseek(nfp, offset, SEEK_SET);
	int c;
	char num[len];
	int ii = 0;
	for(int i = 0; i < len; i++){
		c = fgetc(nfp);
		if(c == EOF){ break; }
		if(c == '\n'){
			array[count] = atoi(num);
			count +=1;
			numindex = 0;
			num[0] = 0;
		}
		num[numindex++] = c;
		if(count >= linestoRead){
			i = len+1;
		}
		ii++;
	}
	write(cd[1], &ii, sizeof(int));
	childProcessMethod(array, count);
	
}


int *childProcessMethod(int *array, int count){
	//Now we can get the total and max from array
	float weightAvg;
	int maxNum;
	int *hiddenkeys;
	int sum;
	int countKeys = 0;
	for(int x = 0; x < count; x++){
		//Hidden keys is going to hold the natural indicies of the negative integers
		if(array[x] == -1){ hiddenkeys[countKeys] = x; }
		if(array[x] > maxNum){
			maxNum = array[x];
		}
		sum = sum + array[x];
	}
	weightAvg = sum/count;
	printf("process %d read %d integers from file\n",getpid(), count);
	printf("found max number %d ", maxNum);
	printf("weighted average found for child process %f \n", weightAvg);
	int *package;
	int packageSize =2;
	package[1] = maxNum;
	package[2] = weightAvg;
	for(int i = 0; i < countKeys; i++){
		package[i+3] = hiddenkeys[i];
		packageSize = packageSize + 1;
	}
	package[0] = packageSize;
	return package;
}

//Method is used to create PN number of child processes and print their PID then prints amount created
//returns the number of child processes created
int createChildProcesses(int pn, pid_t *pid){
	int totalChildProc = 0;
	pipe(fd);
	pipe(cd);
	for(int i = 0; i< pn; i++){
		pid[i] = fork();
		if(pid[i] < 0){
			printf("Something went wrong and child process could not be created\n");
		}else if(pid[i] == 0){
			//Child process code will call a seperate method
			printf("Child process %d has been created\n", getpid());
			childProcessCode(i);
			printf("Child Process %d has terminated\n", getpid());
			exit(10);
		}else{
			//close(fd[0]); //Close reading end of pipe for parent
			//close(cd[1]); //Close writing end of pipe for parent
			//Parent process doesnt do anything but continue the loop
			printf("Child process %d\n", pid[i]);
			totalChildProc = totalChildProc +1;
		}
	}
	printf("%d total processes created\n", totalChildProc);
	return totalChildProc;
}

//Adds randommly generated integer to file
//Adds randomly generated integer to integer array
//Prints out status of lines added vs requested
int addLines(FILE *tfp){
	int r = 0;
	int total = 0;
	int totalHidden =0;
	for(int i=0; i < L; i++){
		r = (rand()+1) % factor; // Add one so we only get values > 0
		if(((r % 5)==0) && totalHidden < atoi(numHiddenKeys)){
			r = -1;
			totalHidden +=1;
		}
		int c = fprintf(tfp, "%d\n", r);
		if( c > 0 ){
			fileArray[i] = r;
			//printf("added %d\n", fileArray[i]);
			total = total + 1;
		}
	}
	printf("Random Number File populated with %d lines out of ", total);
	printf("%d\n",L);
	len = ftell(tfp);
	fclose(tfp);
	return total;
}

//Function can be used to request information from the user
//Conditionals set up for L and H specific user requests
int getUserInput(char *req){
	char *userRequest;
	if(req == "L"){
		userRequest = "Please enter the total number of keys to populate the file with.\n";
	}else if(req == "H"){
		userRequest = "Please enter number of hidden keys to search for out of ";
	}
	int number;
	while(1){
		printf("%s", userRequest);
		if(req == "H") printf("10 \n");
		int r = scanf("%d", &number);
		if(r < 1){
			//Incorrect input
			printf("Input is invalid\n");
			//perform emptyScan to clear stdin buffer of invalid characters
			emptyScan();
		}else{
			//We did get correct input
			if(req == "H"){
				//Check H is not greater than the amount of keys hidden 
				if(number > atoi(numHiddenKeys)){
					printf("Cannot search for more keys than are hidden in the program\n");
				}else{
					return number;
				}
			}else{
				return number;
			}
		}
	}
}


void emptyScan(){
	int c = getchar();
	while( c!= '\n' && c != EOF)
		c = getchar();
}
