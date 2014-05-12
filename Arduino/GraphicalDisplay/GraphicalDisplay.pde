import processing.serial.*;

Serial myPort;  // Create object from Serial class

char nextChar = '\0';
int frequency;

int[] history;

void setup() 
{
  history = new int[100];
  java.util.Arrays.fill(history, 16000);
  size(800, 400);
  myPort = new Serial(this, "COM18", 115200);
  stroke(255);
}

void draw()
{
  int i;

  background(0);

  shiftArray();

  history[99] = getFrequency();
  
  float lastY = map(history[0], 0, 20000, 400, 0);
  
  for (i=1; i < 100; ++i)
  {
    float newY = map(history[i], 0, 20000, 400, 0);
    
    line(i*8, lastY, (i+1)*8, newY);
    lastY = newY;
  }
}

int getFrequency()
{
  int frequency = 0;
  do
  {
    nextChar = (char)myPort.read();
    if ((nextChar >= '0') && (nextChar <= '9'))
    {
      frequency *= 10;
      frequency += (nextChar - '0');         // read it and store it in val
    }
    while (myPort.available () == 0) {
    }
  } 
  while ( nextChar != '\n' );

  return frequency;
}

void shiftArray()
{ 
  int i;
  for (i=0; i < 99; ++i)
  {
    history[i] = history[i+1];
  }
}

