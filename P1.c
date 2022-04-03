#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int H;
int L;
char *numHiddenKeys = "10";
int PN = 4;//THIS SHOULD BE INCREASED FOR BETTER ALGORITHM PERFORMANCE
int fd[2];
//FUNCTION DECLARATIONS
int getUserInput(char *req);
void emptyScan();
int addLines(FILE *tfp, int* fileArray);
int createChildProcesses(int pn, pid_t *pid);
void childProcessCode();
//------------------------------------------

int main(int argc, char *argv[]){
	pid_t pid[PN];
	//Set the seed for the random number
	srand(time(0));
	//First thing needed is to create a file to put all of the lines
	FILE *fp = fopen("RandomNumFile", "wb"); //Create file or open existing file "RandomNumFile", truncate to length 0 for writing
	if(fp == NULL){
		//Something wrong happened
		printf("Could not open or create file");
		exit(10);
	}
	//Here we are going to get L from the user
	L = getUserInput("L");
	//Initialize an array of integers, representation of our file
	int fileArray[L];
	//Here we are going to get H from the user
	H = getUserInput("H");
	int q = addLines(fp, fileArray);
	q = createChildProcesses(PN, pid);
	write(fd[1], fileArray, L);
}


void childProcessCode(){
	int linestoRead = L/PN + 1;
	int array[linestoRead];
	int sumTotal = 0;
	int maxNum = 0;		// maxNum = 0 because all integers should be greater than zero
	//Want the child process to only handle a certain partition of the array. The last child may handle more or less until they reach end of file.
	int n = read(fd[0], array, linestoRead);
	printf("child process %d ", getpid());
	printf("recieved %d bytes\n", n);
	for(int i = 0; i < linestoRead; i++) {
		sumTotal = sumTotal + array[i];
		if(array[i] > maxNum) {
			maxNum = array[i];
		}
	}
}

//Method is used to create PN number of child processes and print their PID then prints amount created
//returns the number of child processes created
int createChildProcesses(int pn, pid_t *pid){
	int totalChildProc =0;
	pipe(fd);
	for(int i = 0; i< pn; i++){
		pid[i] = fork();
		if(pid[i] < 0){
			printf("Something went wrong and child process could not be created\n");
		}else if(pid[i] == 0){
			//Child process code will call a seperate method
			childProcessCode();
			exit(1);
		}else{
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
int addLines(FILE *tfp, int *fileArray){
	int r = 0;
	int total = 0;
	for(int i=0; i < L; i++){
		r = rand() + 1; // Add one so we only get values > 0
		int c = fprintf(tfp, "%d\n", r);
		if( c > 0 ){
			fileArray[i] = r;
			total = total + 1;
		}
	}
	printf("Random Number File populated with %d lines out of ", total);
	printf("%d\n",L);
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
