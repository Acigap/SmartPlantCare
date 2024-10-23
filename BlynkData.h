#include "Arduino.h"

int getSwitchPum();
int getSensorMin();
int getSensorMax();
void setupBlynk();
void loopBlynk();
void virtualWriteV0(int value);
void virtualWriteV2(int value);
void virtualWriteV4(int value);
bool isConnected();