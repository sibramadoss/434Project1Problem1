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
int final[2];
int factor = 10000; //Factor to bind the random numbers by, increase or decrease to change random number range 0 < num < factor + 1
int len;
FILE *fp;
//FUNCTION DECLARATIONS
int getUserInput(char *req);
void emptyScan();
int addLines(FILE *tfp);
int createChildProcesses(int pn, pid_t *pid);
int childProcessCode(int index);
float *childProcessMethod(int *array, int count, int index);
int *fileArray;//Will be the addresses to integers
//------------------------------------------

int main(int argc, char *argv[]){
	double time_spent = 0;
	clock_t begin = clock();
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
	int offset = 0;
	int rec = 0;
	q = createChildProcesses(PN, pid);
	//This allows the child processes to read the file and process the information
	float rem;
	rem = L%PN;
	int tempLength;
	L = L/PN;
	if(rem !=0){ tempLength = L; L = L + rem;}
	for(int w=0; w < PN; w++){
		int package[4] = {L, PN, len, offset};
		L = tempLength;
		write(fd[1],package,4*sizeof(int));
		read(cd[0], &rec, sizeof(int));
		offset = offset + rec;
	}
	float hiddenkeys[atoi(numHiddenKeys)];
	float numberlinesread;
	int count = 0;
	float newpackage[factor];
	float packageSize = 0;
	float finalmaxnum = 0;
	float finalavg = 0;
	for(int r = 0; r < PN; r++){
		read(final[0], &packageSize, sizeof(float));
		read(final[0], &newpackage, (packageSize)*sizeof(float));
		for(int i = 5; i < packageSize; i++){
			hiddenkeys[count] = newpackage[i] + numberlinesread;
			count+=1;
		}
		if(newpackage[3] > finalmaxnum){finalmaxnum = newpackage[3];}
		finalavg = finalavg + newpackage[4] * (float) newpackage[1]/(float)L;
		numberlinesread = numberlinesread + newpackage[1];
	}
	if(count > H) count = H;
	printf("Calculated final average to be %f\n", finalavg);
	printf("Maximum number found was %f\n", finalmaxnum);
	printf("Found %d hidden keys out of %d requested out of %d total hidden keys\n", count, H, atoi(numHiddenKeys));
	for(int p = 0; p < H; p++){
		printf("Key %d found at line : %f\n", (p+1), hiddenkeys[p]);
	}
	clock_t end = clock();
	time_spent += (double)(end-begin)/CLOCKS_PER_SEC;
	printf("Parent process finished, time elapsed %f seconds\n", time_spent);
}


//Returns the number of hidden keys found
int childProcessCode(int index){
	int package[4];
	int n = read(fd[0], &package, 4*sizeof(int));
	int linestoRead = package[0];
	int len = package[2];
	int offset = package[3];
	int count = 0;
	int array[linestoRead];
	int numindex = 0;
	printf("child process %d read data from buffer and will attempt to read %d lines, of the total %d bytes starting from %d\n", getpid(), linestoRead, len, offset);
	fp = fopen("RandomNumFile","r");
	if(fp == NULL){ printf("%d could not open file", getpid()); }
	int c;
	char num[factor];
	int ii = 0;
	for(int i = offset; i <= len; i++){
		fseek(fp, i, SEEK_SET);
		c = fgetc(fp);
		if(feof(fp)){ break; }
		if(c == '\n'){
			num[numindex++] = '\0';
			array[count] = atoi(num);
			count +=1;
			numindex = 0;
			num[0] = 0;
		}else{
			num[numindex++] = c;
		}
		ii = ii + sizeof(char);
		if(count > linestoRead){break;}
	}
	printf("Child process %d read %d lines from file\n", getpid(), count-1);
	write(cd[1], &ii, sizeof(int));
	fclose(fp);
	//Now we can get the total and max from array
	float weightAvg;
	float maxNum = 0;
	float hiddenkeys[factor];
	float sum;
	int countKeys = 0;
	for(int x = 0; x < count; x++){
		//Hidden keys is going to hold the natural indicies of the negative integers
		if(array[x] > maxNum){
			maxNum = array[x];
		}
		if(array[x] == -1){
			hiddenkeys[countKeys] = (float)x;
			countKeys++;
		}
		sum = sum + array[x];
	}
	weightAvg = (sum/(float)count);
	float newpackage[factor];
	float packageSize = 5;
	newpackage[1] = (float) count;
	newpackage[2] = (float) index;
	newpackage[3] = maxNum;
	newpackage[4] = weightAvg;
	for(int i = 0; i < countKeys; i++){
		newpackage[i+5] = hiddenkeys[i];
		packageSize = packageSize + 1;
	}
	newpackage[0] = packageSize;
	int byte;
	write(final[1], &packageSize, sizeof(float));
	write(final[1], newpackage, packageSize * sizeof(float));
}

//Method is used to create PN number of child processes and print their PID then prints amount created
//returns the number of child processes created
int createChildProcesses(int pn, pid_t *pid){
	int totalChildProc = 0;
	pipe(fd);
	pipe(cd);
	pipe(final);
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
		int c = fprintf(tfp, "%d\n", r);
		if( c > 0 ){
			fileArray[i] = r;
			//printf("added %d\n", fileArray[i]);
			total = total + 1;
		}
	}
	printf("Random Number File populated with %d lines out of ", total);
	len = ftell(tfp);
	printf("%d\n",L);
	int x =0;
	while(x < atoi(numHiddenKeys)){
		r = (rand() + 1) % len-1;
		fseek(tfp, r, SEEK_SET);
		int c2 = fprintf(tfp, "\n-1\n");
		if(c2 > 0){ x++; }
	}
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
