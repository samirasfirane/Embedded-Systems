/*----------------------------------------------------------------------------
Final Project - TRAFFIC SIGNAL
PROGRAMMING USING MBED API- Samir Asfirane, Erik Kalan, Javier Valerio.
------------------------------------------------------------------------
Final Project
*----------------------------------------------------------------------------*/

#include "mbed.h"
#include "rtos.h"
#include "DS1631.h"
#include "pindef.h"
#include <stdio.h>
#include <time.h>

DigitalIn btn_intrpt(PA_10);

DigitalOut myledRED1(PB_3);  /// Connect LED cathode to pin PB_3
DigitalOut myledGREEN1(PB_4);  /// Connect LED cathode to pin PB_4
DigitalOut myledORANGE1(PB_5);  /// Connect LED cathode to pin PB_5
DigitalOut myledRED2(PB_10);  /// Connect LED cathode to pin PB_10
DigitalOut myledGREEN2(PA_9);  /// Connect LED cathode to pin PA_9
DigitalOut myledORANGE2(PA_8);  /// Connect LED cathode to pin PA_8

//I2C interface
I2C temp_sensor(I2C_SDA, I2C_SCL);
Serial pc(UART_TX, UART_RX);

PwmOut speaker1(PA_7); /// Connect speaker to pin PA_7 (this speaker sounds when light1 is green)
PwmOut speaker2(PB_6); /// Connect speaker to pin PB_6 ( one of the led should be changed so the speaker works) this speaker sounds when light2 is green

//  Struct for temp sensor
typedef struct {
    int id;
    int addr;
    int temp;
} TempData;


float temp1 = 0, temp2 = 23.6; // two floats represeting each the temperatures of the sensors
Mutex light1_mutex; // mutex for traffic light 1
Mutex light2_mutex; // mutex for traffic light 2
Mutex sound_mutex;  // mutex for pedestrian signal
Mutex serial_mutex; // mutex for serial transmision

//I2C address of temperature sensor DS1631
const int temp_addr = 0x90;
char cmd[] = { 0x51, 0xAA };
char read_temp[2];

int seconds = 10;
time_t endwait = time (NULL) + seconds ;

// function that takes a float argument and turns LED GREEN
// for whatever amount of time passed as argument
void lightGREEN1(float time)
{

    myledGREEN1 = 1;
    wait(time);
    myledGREEN1 = 0;

}
// function that takes a float argument and turns LED orange(blue)
// for whatever amount of time passed as argument
void lightORANGE1(float time)
{

    myledORANGE1 = 1;
    wait(time);
    myledORANGE1 = 0;

}
// function that takes a float argument and turns LED red
// for whatever amount of time passed as argument
void lightRED1(float time)
{

    myledRED1 = 1;
    wait(time);
    myledRED1 = 0;

}
// function that takes a float argument and turns LED GREEN
// for whatever amount of time passed as argument
void lightGREEN2(float time)
{

    myledGREEN2 = 1;
    wait(time);
    myledGREEN2 = 0;

}
// function that takes a float argument and turns LED orange(blue)
// for whatever amount of time passed as argument
void lightORANGE2(float time)
{

    myledORANGE2 = 1;
    wait(time);
    myledORANGE2 = 0;

}
// function that takes a float argument and turns LED red
// for whatever amount of time passed as argument
void lightRED2(float time)
{

    myledRED2 = 1;
    wait(time);
    myledRED2 = 0;

}
// routine A for the thread on the first light
void routineALight1()
{
    // routine that light red for 10 seconds, orange for 2 and then red for 2 seconds
    light1_mutex.lock();

    if (temp1 >= temp2) {
        lightGREEN1(10.0);
        lightORANGE1(2.0);
        lightRED1(5.0);
    }

    light1_mutex.unlock();
}
// routine A for the thread on the second light
void routineALight2()
{
    // routine that light red for 12 seconds, green for 3 and then orange for 2 seconds
    light2_mutex.lock();

    if (temp1 >= temp2) {
        lightRED2(12.0);
        lightGREEN2(3.0);
        lightORANGE2(2.0);
    }

    light2_mutex.unlock();

}
// routine B for the thread on the first light
void routineBLight1()
{
    // routine that light red for 12 seconds, green for 3 and then orange for 2 seconds
    light1_mutex.lock();

    if (temp1<temp2) {
        lightRED1(10.0);
        lightGREEN1(3.0);
        lightORANGE1(2.0);
        lightRED1(2.0);
    }

    light1_mutex.unlock();
}
// routine B for the thread on the second light
void routineBLight2()
{
    // routine that light red for 10 seconds, Orange for 2 and then red for 5 seconds
    light2_mutex.lock();

    if (temp1<temp2) {
        lightGREEN2(8.0);
        lightORANGE2(2.0);
        lightRED2(7.0);
    }

    light2_mutex.unlock();

}
// A sound signal for pedestrian to cross takes two arguments a float
// for the time it needs to stay on and a speaker to use for the sound
void pedestrianSound(float time, PwmOut speaker)
{
    // lock the mutex for the pedestrian signal
    sound_mutex.lock();
    while (time>0) {
        int i = 0;
        // generate a short 150Hz tone using PWM hardware output
        // something like this can be used for a button click effect for feedback
        for (i = 0; i<10; i++) {
            speaker.period(1.0 / 150.0); // 500hz period
            speaker = 0.01; //1% duty cycle - High range volume
            wait(.02);
            speaker = 0.0; // turn off audio
            wait(0.5);
            time = time - (float)0.5;
        }

    }
    // unlock the mutex for the pedestrian signal
    sound_mutex.unlock();

}
// runs pedestrian signal for a while then waits for a time
// before running again in an infinite loop
void pedSoundAlight1(void const *args)
{
    // routine that light red for 10 seconds, Orange for 2 and then red for 5 seconds
    if (temp1 >= temp2) {
        while (true) {
            pedestrianSound(10.0, speaker1);
            wait(7.0);
            Thread::wait(200);
        }
    }
}

// runs pedestrian signal for a while then waits for a time
// before running again in an infinite loop
void pedSoundAlight2(void const *args)
{
    // if temp1 >= temp2 means light one is green for 10 seconds
    if (temp1 >= temp2) {
        while (true) {
            wait(12.0);
            pedestrianSound(3.0, speaker2);
            wait(2.0);
            Thread::wait(200);
        }
    }
}

// runs pedestrian signal for a while then waits for a time
// before running again in an infinite loop
void pedSoundBlight1(void const *args)
{
    if (temp1 < temp2) {
        while (true) {
            wait(10.0);
            pedestrianSound(3.0, speaker1);
            wait(4.0);
            Thread::wait(200);
        }
    }
}

// runs pedestrian signal for a while then waits for a time
// before running again in an infinite loop
void pedSoundBlight2(void const *args)
{
    // if temp1 >= temp2 means light one is green for 10 seconds
    if (temp1<temp2) {
        while (true) {
            pedestrianSound(8.0, speaker2);
            wait(9.0);
            Thread::wait(200);
        }
    }
}
// runs routine A for light 1 in a loop
void runRoutineALight1(void const *args)
{
    // routine that light red for 10 seconds, Orange for 2 and then red for 5 seconds
    while (true) {
        routineALight1();
        Thread::wait(200);
    }
}
// runs routine A for light 2 in a loop
void runRoutineALight2(void const *args)
{
    // routine that light red for 10 seconds, Orange for 2 and then red for 5 seconds
    while (true) {
        routineALight2();
        Thread::wait(200);
    }
}

// runs routine B for light 1 in a loop
void runRoutineBLight1(void const *args)
{
    // routine that light red for 10 seconds, Orange for 2 and then red for 5 seconds
    while (true) {
        routineBLight1();
        Thread::wait(200);
    }
}
// runs routine B for light 2 in a loop
void runRoutineBLight2(void const *args)
{
    // routine that light red for 10 seconds, Orange for 2 and then red for 5 seconds
    while (true) {
        routineBLight2();
        Thread::wait(200);
    }
}
// temperature thread reads the temperature every second and updates the temp1
void temp_thread(void const *args)
{
    //write your code here
    while (1) {

        //Sensor temperature reading the temperature every second
        wait(1.0);
        temp_sensor.write(temp_addr, &cmd[0], 1);
        temp_sensor.write(temp_addr, &cmd[1], 1);
        temp_sensor.read(temp_addr, read_temp, 2);

        temp1 = (float((read_temp[0] << 8) | read_temp[1]) / 256);

        temp_sensor.write(temp_addr + 6, &cmd[0], 1);
        temp_sensor.write(temp_addr + 6, &cmd[1], 1);
        temp_sensor.read(temp_addr + 6, read_temp, 2);

        temp1 += (float((read_temp[0] << 8) | read_temp[1]) / 256);
        temp1 /= 2;

        temp_sensor.write(temp_addr + 2, &cmd[0], 1);
        temp_sensor.write(temp_addr + 2, &cmd[1], 1);
        temp_sensor.read(temp_addr + 2, read_temp, 2);

        temp2 = (float((read_temp[0] << 8) | read_temp[1]) / 256);

        temp_sensor.write(temp_addr + 4, &cmd[0], 1);
        temp_sensor.write(temp_addr + 4, &cmd[1], 1);
        temp_sensor.read(temp_addr + 4, read_temp, 2);

        temp2 += (float((read_temp[0] << 8) | read_temp[1]) / 256);
        temp2 /= 2;

        serial_mutex.lock();

        pc.printf ("t1: %f\n\r", temp1);
        pc.printf ("t2: %f\n\r", temp2);

        serial_mutex.unlock();

        wait(2.0);

        Thread::wait(600);
    }
}

void traffic_interrupt ( )
{

    time_t endwait = time (NULL) + seconds ;

    if (!btn_intrpt) {

        pc.printf ("State: %s\n\r", "Interrumpted");

        while (time (NULL) < endwait) {

            myledRED1 = 1;
            myledRED2 = 1;

            wait (0.6);

            myledRED1 = 0;
            myledRED2 = 0;

            myledGREEN1 = 0;
            myledGREEN2 = 0;

            myledORANGE1 = 0;
            myledORANGE2 = 0;
            wait (0.6);
        }
    }
}

int main()
{
    pc.printf ("Starting %s\n\r", "Application");

    Thread tempThread (temp_thread);          // thread that will calculate the temperature every second

    Thread firstlight1 (runRoutineALight1);   // thread that will run light 1 if temp1>=temp2
    Thread soundAlight1 (pedSoundAlight1);
    Thread secondlight2 (runRoutineALight2);  // thread that will run light 2 if temp1>=temp2
    Thread soundAlight2 (pedSoundAlight2);

    Thread firstlight3 (runRoutineBLight1);   // thread that will run light 1 if temp1<temp2
    Thread soundBlight1 (pedSoundBlight1);
    Thread secondlight4 (runRoutineBLight2);  // thread that will run light 2 if temp1<temp2
    Thread soundBlight2 (pedSoundBlight2);

    while (1) {
		traffic_interrupt ();
    }

}
