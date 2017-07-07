#include <sys/timeb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
long gRefTime;


struct insertArray{
	int *arr;
	int arrSize;
	int threadSize;
	int threadInit;
	int threadPoint;

};
long GetMilliSecondTime(struct timeb timeBuf)
{
	long mliScndTime;

	mliScndTime = timeBuf.time;
	mliScndTime *= 1000;
	mliScndTime += timeBuf.millitm;
	return mliScndTime;
}


long GetCurrentTime(void)
{
	long crntTime=0;
	struct timeb timeBuf;
	ftime(&timeBuf);
	crntTime = GetMilliSecondTime(timeBuf);
	return crntTime;
}

void SetTime(void)
{
	gRefTime = GetCurrentTime();
}

long GetTime(void)
{
	long crntTime = GetCurrentTime();
	return (crntTime - gRefTime);
}

void InsertionSort(int data[], int size)
{
	int i, j, temp;
	for(i=1; i<size; i++){
		temp = data[i];
		for(j=i-1; j>=0 && data[j]>temp; j--)
			data[j+1] = data[j];
		data[j+1] = temp;
	}
}
int Rand(int x, int y)
{
	int range = y-x+1;
	int r = rand() % range;
	r += x;
	return r;
}
void Swap(int& x, int& y)
{
	int temp = x;
	x = y;
	y = temp;
}
void PrintArray(int b[], int size){
	for (int i = 0; i <size; i++){
		printf("%d\n",b[i]);
	}

}


int Partition(int data[], int p, int r)
{
	int i, j, x, pi;
	pi = Rand(p, r);
	Swap(data[r], data[pi]);
	x = data[r];
	i = p-1;
	for(j=p; j<r; j++){
		if(data[j] < x){
			i++;
			Swap(data[i], data[j]);
		}
	}
	Swap(data[i+1], data[r]);
	return i+1;
}


void QuickSort(int data[], int p, int r)
{
	int q;
	if(p >= r) return;
	q = Partition(data, p, r);
	QuickSort(data, p, q-1);
	QuickSort(data, q+1, r);
}
int *CreateArray(int num){
	int i=0;
	int *pty;
	pty = (int *)malloc(sizeof(int)*num);
	if(pty!=NULL){
		for(i=0; i<num; i++){
			pty[i] = 0;
		}
		
	}
	return pty;

}
int g = 0;
void *multiThreadQ(void *vargp){
		struct insertArray *newA= (struct insertArray *)vargp;
		int *newThreadA;
		newThreadA = CreateArray(newA->threadPoint);
		for(int z = newA->threadInit; z <newA->threadPoint; z++){
			newThreadA[z] = newA->arr[z];
		}
		
		
		QuickSort(newThreadA,newA->threadInit, newA->threadPoint );
		for (int z = newA->threadInit; z < newA->threadPoint; z++){
			newA->arr[z] = newThreadA[z];
		}
		free(newThreadA);

	}
void *multiThreadI(void *vargp){
		struct insertArray *newA= (struct insertArray *)vargp;
		int *newThreadA;
		newThreadA = CreateArray(newA->threadPoint);
		for(int z = newA->threadInit; z <newA->threadPoint; z++){
			newThreadA[z] = newA->arr[z];
		}
		
		
		InsertionSort(newThreadA,newA->threadPoint );
		for (int z = newA->threadInit; z < newA->threadPoint; z++){
			newA->arr[z] = newThreadA[z];
		}
		free(newThreadA);
		
	}

int main(int argc, char *argv[]){
	
	
	


	
	int e;

	int first = atoi(argv[1]);
	
	int second = atoi(argv[2]);
	int finalArray[second*2];
	int indicesPoints[first];	
	int myArray[first];
	int otherArray[first];
	pthread_t multiT[second];
	if (first > 0 && first <=100000000){
		first=first;
	}
	else{
		printf("Array size failed please specify between 1 and 100000k\n");
		exit(EXIT_FAILURE);
	}
	if (second >0 && second <=16){
		second=second;	
	}
	else{
		printf("Thread size failed please specify between 1 and 16\n");
		exit(EXIT_FAILURE);
	}

	for (e=0; e< first; e++){
		myArray[e] = Rand(0,first);

	}
	int k;
	int tempNumb = first;
	tempNumb= tempNumb/second;
	tempNumb= tempNumb-1;
	indicesPoints[0] = 0;
	indicesPoints[1] = tempNumb;
	printf("%d\n",indicesPoints[0]);
	printf("%d\n",indicesPoints[1]);
	for(k=2; k<(second*2)-k+1; k++){
		indicesPoints[k] = (tempNumb*k)+k-1;
		printf("%d\n",indicesPoints[k]);
	}
	int m;
	
	printf("test\n");
	SetTime();
	char* sortStyle = argv[3];
	if(strcmp("q",argv[3])==0){
		printf("here\n");
		for(m=0; m<second; m++){
			QuickSort(myArray,indicesPoints[m],indicesPoints[m+1]);
			
		}
	
	}
	if(strcmp("i",argv[3])==0){
		printf("here2\n");
		for(m=0; m<second; m++){
			InsertionSort(myArray,first);
			
		}
	
	}
	int i;
	int j;
	int a;
	
	for (i=0; i< first; i++){
		for (j=i+1; j<first; j++){
			if(myArray[i] > myArray[j]){
				a= myArray[i];
				myArray[i] = myArray[j];
				myArray[j] = a;		
			}
		
		}
	}
	
	printf("Correctly Sorted\n");
	printf("Time: %d\n",GetTime());
	PrintArray(myArray,first);

	
	for (e=0; e< first; e++){
		otherArray[e] = Rand(0,10000000);

	}
	
	/*Multithread area, creates multiple threads based on input*/
	struct insertArray *threadArr;
	
	int numHold= indicesPoints[1];
	threadArr = (struct insertArray *)malloc(sizeof(struct insertArray));

	threadArr->arr = otherArray;
	threadArr->arrSize= first;
	threadArr->threadSize=second;
	threadArr->threadPoint = numHold;
	/*for (e=0; e <first; e++){
	printf("%d\n", threadArr->arr[e]);
	}*/
	SetTime();
	for (e = 0; e < second; e++){
		if(strcmp("q",argv[3])==0){
		pthread_create(&multiT[e], NULL, &multiThreadQ, (void *) threadArr);
		threadArr->threadInit = threadArr->threadPoint;
		
		threadArr->threadPoint = threadArr->threadPoint + numHold ;
		
		
		}

		if(strcmp("i",argv[3])==0){
		pthread_create(&multiT[e], NULL, &multiThreadI, (void *) threadArr);
		threadArr->threadInit = threadArr->threadPoint;
		
		threadArr->threadPoint = threadArr->threadPoint + numHold ;
		}
	for (e=0; e <first; e++){
	printf("%d\n\n", threadArr->arr[e]);
	}
	
	

	}
	for(int x = 0; x<second; x++){
			pthread_join(multiT[x],NULL);
		}
	printf("Time: %d\n",GetTime());	
	printf("Sorted\n");
	
	pthread_exit(NULL);
	
	

	return 0;
}



