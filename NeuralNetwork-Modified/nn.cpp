/**************************************************
  Neural Network with Backpropagation
  --------------------------------------------------
  Adapted from D. Whitley, Colorado State University
  Modifications by S. Gordon
  --------------------------------------------------
  Version 4.0 - November 2012
    scaling and squashing function restructured
  --------------------------------------------------
  compile with g++ nn.c
****************************************************/

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
using namespace std;

#define NumOfCols    6       /* number of layers +1  i.e, include input layer */
#define NumOfRows    10      /* max number of rows net +1, last is bias node  */
#define NumINs       2       /* number of inputs, not including bias node     */
#define NumOUTs      1       /* number of outputs, not including bias node    */
#define LearningRate 0.2  /* most books suggest 0.3                        */
#define Criteria     1000.0   /* all outputs must be within this to terminate  */
#define TestCriteria 1000.0     /* all outputs must be within this to generalize */
#define MaxIterate   100  /* maximum number of iterations                */
#define ReportIntv   1     /* print report every time this many cases done*/
#define Momentum     0.9     /* momentum constant                             */
#define TrainCases   35      /* number of training cases        */
#define TestCases    6       /* number of test cases            */
// network topology by column ------------------------------------
#define NumNodes1    3       /* col 1 - must equal NumINs+1     */
#define NumNodes2    10       /* col 2 - hidden layer 1, etc.    */
#define NumNodes3    10       /* output layer must equal NumOUTs */
#define NumNodes4    10       /*                                 */
#define NumNodes5    10       /* note: layers include bias node  */
#define NumNodes6    1
#define TrainFile    "trainpart3ke.dat"  /* file containing training data */
#define TestFile     "testpart3ke.dat"   /* file containing testing data  */

int NumRowsPer[NumOfRows];  /* number of rows used in each column incl. bias */
                            /* note - bias is not included on output layer   */
                            /* note - leftmost value must equal NumINs+1     */
                            /* note - rightmost value must equal NumOUTs     */

double TrainArray[TrainCases][NumINs + NumOUTs];
double TestArray[TestCases][NumINs + NumOUTs];
int CritrIt = 3 * TrainCases;

ifstream train_stream;      /* source of training data */
ifstream test_stream;       /* source of test data     */

void CalculateInputsAndOutputs ();
void TestInputsAndOutputs();
void TestForward();
double ScaleOutput(double X, int which);
double ScaleDown(double X, int which);
void GenReport(int Iteration);
void TrainForward();
void FinReport(int Iteration);
void DumpWeights();
double squashing(double Sum);
double Dsquashing(double out);
void ScaleCriteria();

struct CellRecord
{
  double Output;
  double Error;
  double Weights[NumOfRows];
  double PrevDelta[NumOfRows];
};

struct CellRecord  CellArray[NumOfRows][NumOfCols];
double Inputs[NumINs];
double DesiredOutputs[NumOUTs];
double extrema[NumINs+NumOUTs][2];  // [0] is low, [1] is hi
long   Iteration;
double ScaledCriteria[NumOUTs], ScaledTestCriteria[NumOUTs];

/************************************************************
   Get data from Training and Testing Files, put into arrays
*************************************************************/
void GetData()
{
  for (int i=0; i < (NumINs+NumOUTs); i++)
  { extrema[i][0]=99999.0; extrema[i][1]=-99999.0; }

  // read in training data
  train_stream.open(TrainFile);
  for (int i=0; i < TrainCases; i++)
  { for (int j=0; j < (NumINs+NumOUTs); j++)
    { train_stream >> TrainArray[i][j];
      if (TrainArray[i][j] < extrema[j][0]) extrema[j][0] = TrainArray[i][j];
      if (TrainArray[i][j] > extrema[j][1]) extrema[j][1] = TrainArray[i][j];
  } }
  train_stream.close();

  // read in test data
  test_stream.open(TestFile);
  for (int i=0; i < TestCases; i++)
  { for (int j=0; j < (NumINs+NumOUTs); j++)
    { test_stream >> TestArray[i][j];
      if (TestArray[i][j] < extrema[j][0]) extrema[j][0] = TestArray[i][j];
      if (TestArray[i][j] > extrema[j][1]) extrema[j][1] = TestArray[i][j];
  } }

  // guard against both extrema being equal
  for (int i=0; i < (NumINs+NumOUTs); i++)
    if (extrema[i][0] == extrema[i][1]) extrema[i][1]=extrema[i][0]+1;
  test_stream.close();

  // scale training and test data to range 0..1
  for (int i=0; i < TrainCases; i++)
    for (int j=0; j < NumINs+NumOUTs; j++)
      TrainArray[i][j] = ScaleDown(TrainArray[i][j],j);
  for (int i=0; i < TestCases; i++)
    for (int j=0; j < NumINs+NumOUTs; j++)
      TestArray[i][j] = ScaleDown(TestArray[i][j],j);
}

/**************************************************************
   Assign the next training pair
***************************************************************/
void CalculateInputsAndOutputs()
{
  static int S=0;
  for (int i=0; i < NumINs; i++) Inputs[i]=TrainArray[S][i];
  for (int i=0; i < NumOUTs; i++) DesiredOutputs[i]=TrainArray[S][i+NumINs];
  S++;
  if (S==TrainCases) S=0;
}

/**************************************************************
   Assign the next testing pair
***************************************************************/
void TestInputsAndOutputs()
{
  static int S=0;
  for (int i=0; i < NumINs; i++) Inputs[i]=TestArray[S][i];
  for (int i=0; i < NumOUTs; i++) DesiredOutputs[i]=TestArray[S][i+NumINs];
  S++;
  if (S==TestCases) S=0;
}

/*************************   MAIN   *************************************/

int main()
{
  int    I, J, K, existsError, ConvergedIterations=0;
  long   seedval;
  double Sum, newDelta;

  Iteration=0;

  NumRowsPer[0] = NumNodes1;  NumRowsPer[3] = NumNodes4;
  NumRowsPer[1] = NumNodes2;  NumRowsPer[4] = NumNodes5;
  NumRowsPer[2] = NumNodes3;  NumRowsPer[5] = NumNodes6;

  /* initialize the weights to small random values. */
  /* initialize previous changes to 0 (momentum).   */
  seedval = 555;
  srand(seedval);
  for (I=1; I < NumOfCols; I++)
    for (J=0; J < NumRowsPer[I]; J++)
      for (K=0; K < NumRowsPer[I-1]; K++)
      { CellArray[J][I].Weights[K] =
        2.0 * ((double)((int)rand() % 100000 / 100000.0)) - 1.0;
        CellArray[J][I].PrevDelta[K] = 0;
      }

  GetData();  // read training and test data into arrays
  ScaleCriteria();

  cout << endl << "Iteration     Inputs          ";
  cout << "Desired Outputs          Actual Outputs" << endl;

  // -------------------------------
  // beginning of main training loop
  do
  { /* retrieve a training pair */
    CalculateInputsAndOutputs();
    for (J=0; J < NumRowsPer[0]-1; J++) CellArray[J][0].Output = Inputs[J];

    /* set up bias nodes */
    for (I=0; I < NumOfCols-1; I++)
    { CellArray[NumRowsPer[I]-1][I].Output = 1.0;
      CellArray[NumRowsPer[I]-1][I].Error = 0.0;
    }

    /**************************
     *    FORWARD PASS        *
     **************************/
    /* hidden layers */
    for (I=1; I < NumOfCols-1; I++)
      for (J=0; J < NumRowsPer[I]-1; J++)
      { Sum = 0.0;
        for (K=0; K < NumRowsPer[I-1]; K++)
          Sum += CellArray[J][I].Weights[K] * CellArray[K][I-1].Output;
        CellArray[J][I].Output = squashing(Sum);
        CellArray[J][I].Error = 0.0;
      }

    /* output layer  */
    for (J=0; J < NumOUTs; J++)
    { Sum = 0.0;
      for (K=0; K < NumRowsPer[NumOfCols-2]; K++)
        Sum += CellArray[J][NumOfCols-1].Weights[K]
             * CellArray[K][NumOfCols-2].Output;
      CellArray[J][NumOfCols-1].Output = squashing(Sum);
      CellArray[J][NumOfCols-1].Error = 0.0;
    }

    /**************************
     *    BACKWARD PASS       *
     **************************/
    /* calculate error at each output node */
    for (J=0; J < NumOUTs; J++)
      CellArray[J][NumOfCols-1].Error =
        DesiredOutputs[J]-CellArray[J][NumOfCols-1].Output;

    /* check to see how many consecutive oks seen so far */
    existsError = 0;
    for (J=0; J < NumOUTs; J++)
    { if (fabs(CellArray[J][NumOfCols-1].Error) > ScaledCriteria[J])
        existsError = 1;
    }
    if (existsError == 0) ConvergedIterations++;
    else ConvergedIterations = 0;

    /* apply derivative of squashing function to output errors */
    for (J=0; J < NumOUTs; J++)
      CellArray[J][NumOfCols-1].Error =
          CellArray[J][NumOfCols-1].Error
        * Dsquashing(CellArray[J][NumOfCols-1].Output);

    /* backpropagate error */
    /* output layer */
    for (J=0; J < NumRowsPer[NumOfCols-2]; J++)
      for (K=0; K < NumRowsPer[NumOfCols-1]; K++)
        CellArray[J][NumOfCols-2].Error = CellArray[J][NumOfCols-2].Error
        + CellArray[K][NumOfCols-1].Weights[J]
        * CellArray[K][NumOfCols-1].Error
        * Dsquashing(CellArray[J][NumOfCols-2].Output);

    /* hidden layers */
    for (I=NumOfCols-3; I>=0; I--)
      for (J=0; J < NumRowsPer[I]; J++)
        for (K=0; K < NumRowsPer[I+1]-1; K++)
          CellArray[J][I].Error =
          CellArray[J][I].Error
          + CellArray[K][I+1].Weights[J] * CellArray[K][I+1].Error
          * Dsquashing(CellArray[J][I].Output);

    /* adjust weights */
    for (I=1; I < NumOfCols; I++)
      for (J=0; J < NumRowsPer[I]; J++)
        for (K=0; K < NumRowsPer[I-1]; K++)
        { newDelta = (Momentum * CellArray[J][I].PrevDelta[K])
          + (LearningRate * CellArray[K][I-1].Output * CellArray[J][I].Error);
          CellArray[J][I].Weights[K] = CellArray[J][I].Weights[K] + newDelta;
          CellArray[J][I].PrevDelta[K] = newDelta;
        }

    GenReport(Iteration);
    Iteration++;
  } while (!((ConvergedIterations >= CritrIt) || (Iteration >= MaxIterate)));
  // end of main training loop
  // -------------------------------

  FinReport(ConvergedIterations);
  TrainForward();
  TestForward();
  return(0);
}

/*******************************************
   Run Test Data forward pass only
*******************************************/
void TestForward()
{
  int GoodCount=0;
  double Sum, TotalError=0;
  cout << "Running Test Cases" << endl;
  for (int H=0; H < TestCases; H++)
  { TestInputsAndOutputs();
    for (int J=0; J < NumRowsPer[0]-1; J++) CellArray[J][0].Output = Inputs[J];

    /* hidden layers */
    for (int I=1; I < NumOfCols-1; I++)
      for (int J=0; J < NumRowsPer[I]-1; J++)
      { Sum = 0.0;
        for (int K=0; K < NumRowsPer[I-1]; K++)
          Sum += CellArray[J][I].Weights[K] * CellArray[K][I-1].Output;
        CellArray[J][I].Output = squashing(Sum);
        CellArray[J][I].Error = 0.0;
      }

    /* output layer  */
    for (int J=0; J < NumOUTs; J++)
    { Sum = 0.0;
      for (int K=0; K < NumRowsPer[NumOfCols-2]; K++)
        Sum += CellArray[J][NumOfCols-1].Weights[K]
             * CellArray[K][NumOfCols-2].Output;
      CellArray[J][NumOfCols-1].Output = squashing(Sum);
      CellArray[J][NumOfCols-1].Error = 
        DesiredOutputs[J]-CellArray[J][NumOfCols-1].Output;
      if (fabs(CellArray[J][NumOfCols-1].Error) <= ScaledTestCriteria[J])
         GoodCount++;
      TotalError += CellArray[J][NumOfCols-1].Error *
                    CellArray[J][NumOfCols-1].Error;
    }
    GenReport(-1);
  }
  cout << endl;
  cout << "Sum Squared Error for Testing cases   = " << TotalError << endl;
  cout << "% of Testing Cases that meet criteria = " <<
              ((double)GoodCount/(double)TestCases);
  cout << endl;
  cout << endl;
}

/*****************************************************
   Run Training Data forward pass only, after training
******************************************************/
void TrainForward()
{
  int GoodCount=0;
  double Sum, TotalError=0;
  cout << endl << "Confirm Training Cases" << endl;
  for (int H=0; H < TrainCases; H++)
  { CalculateInputsAndOutputs ();
    for (int J=0; J < NumRowsPer[0]-1; J++) CellArray[J][0].Output = Inputs[J];

    /* hidden layers */
    for (int I=1; I < NumOfCols-1; I++)
      for (int J=0; J < NumRowsPer[I]-1; J++)
      { Sum = 0.0;
        for (int K=0; K < NumRowsPer[I-1]; K++)
          Sum += CellArray[J][I].Weights[K] * CellArray[K][I-1].Output;
        CellArray[J][I].Output = squashing(Sum);
        CellArray[J][I].Error = 0.0;
      }

    /* output layer  */
    for (int J=0; J < NumOUTs; J++)
    { Sum = 0.0;
      for (int K=0; K < NumRowsPer[NumOfCols-2]; K++)
        Sum += CellArray[J][NumOfCols-1].Weights[K]
             * CellArray[K][NumOfCols-2].Output;
      CellArray[J][NumOfCols-1].Output = squashing(Sum);
      CellArray[J][NumOfCols-1].Error =
        DesiredOutputs[J]-CellArray[J][NumOfCols-1].Output;
      if (fabs(CellArray[J][NumOfCols-1].Error) <= ScaledCriteria[J])
         GoodCount++;
      TotalError += CellArray[J][NumOfCols-1].Error *
                    CellArray[J][NumOfCols-1].Error;
    }
    GenReport(-1);
  }
  cout << endl;
  cout << "Sum Squared Error for Training cases   = " << TotalError << endl;
  cout << "% of Training Cases that meet criteria = " <<
              ((double)GoodCount/(double)TrainCases) << endl;
  cout << endl;
}

/*******************************************
   Final Report
*******************************************/
void FinReport(int CIterations)
{
  cout.setf(ios::fixed); cout.setf(ios::showpoint); cout.precision(4);
  if (CIterations<CritrIt) cout << "Network did not converge" << endl;
  else cout << "Converged to within criteria" << endl;
  cout << "Total number of iterations = " << Iteration << endl;
}

/*******************************************
   Generation Report
   pass in a -1 if running test cases
*******************************************/
void GenReport(int Iteration)
{
  int J;
  cout.setf(ios::fixed); cout.setf(ios::showpoint); cout.precision(4);
  if (Iteration == -1)
  { for (J=0; J < NumRowsPer[0]-1; J++)
      cout << " " << ScaleOutput(Inputs[J],J);
    cout << "  ";
    for (J=0; J < NumOUTs; J++)
       cout << " " << ScaleOutput(DesiredOutputs[J],NumINs+J);
    cout << "  ";
    for (J=0; J < NumOUTs; J++)
      cout << " " << ScaleOutput(CellArray[J][NumOfCols-1].Output,NumINs+J);
    cout << endl;
  }
  else if ((Iteration % ReportIntv) == 0)
  { cout << "  " << Iteration << "  ";
    for (J=0; J < NumRowsPer[0]-1; J++)
      cout << " " << ScaleOutput(Inputs[J],J);
    cout << "  ";
    for (J=0; J < NumOUTs; J++) 
      cout << " " << ScaleOutput(DesiredOutputs[J],NumINs+J);
    cout << "  ";
    for (J=0; J < NumOUTs; J++)
      cout << " " << ScaleOutput(CellArray[J][NumOfCols-1].Output,NumINs+J);
    cout << endl;
  }
}

/**********************************************
    Squashing Function
***********************************************/
double squashing(double Sum)
{ return 1.0/(1.0+exp(-Sum));
}

/**********************************************
    Derivative of Squashing Function
***********************************************/
double Dsquashing(double out)
{ return out * (1.0-out);
}

/*******************************************
 *    Scale Desired Output 
 ********************************************/
double ScaleDown(double X, int output)
{ return .9*(X-extrema[output][0])/(extrema[output][1]-extrema[output][0])+.05;
} 

/*******************************************
 *    Scale actual output to original range
 ********************************************/
double ScaleOutput(double X, int output)
{
  double range = extrema[output][1] - extrema[output][0];
  double scaleUp = ((X-.05)/.9) * range;
  return (extrema[output][0] + scaleUp);
}

/*******************************************
 *     Scale criteria
 *******************************************/
void ScaleCriteria()
{ int J;
  for (J=0; J < NumOUTs; J++)
    ScaledCriteria[J] = .9*Criteria/(extrema[NumINs+J][1]-extrema[NumINs+J][0]);
  for (J=0; J < NumOUTs; J++)
    ScaledTestCriteria[J] = .9*TestCriteria/(extrema[NumINs+J][1]-extrema[NumINs+J][0]);
}

