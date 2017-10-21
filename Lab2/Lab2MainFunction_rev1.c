/*
 * File:   PIC and Galileo communication               
 * 
 * simple Galileo program example for main function
 * for UMass Lowell 16.480/552
 * 
 * Author: Jose Velis, Andy MacGregor,Grayson Colwell
 * Lab 2 main function Rev_1
 *
 * Created on 10/17/2017
 */


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


//Linux GPIO number // Shield Pin Name
#define Strobe                  (40) // 8
#define GP_4                    (48) // A0
#define GP_5                    (50) // A1
#define GP_6                    (52) // A2
#define GP_7                    (54) // A3

#define GPIO_DIRECTION_IN       (1)//Go HIGH (acoording to handout timing diagram))
#define GPIO_DIRECTION_OUT      (0)//Go LOW (acoording to handout timing diagram))
#define HIGH                    (1)
#define LOW                     (0)

#define SUCCESS                 (0)
#define ERROR                   (-1)

/*User Commands*/
#define MSG_RESET   0x1
#define MSG_PING    0x2
#define MSG_GET     0x3
#define MSG_TURN30  0x4
#define MSG_TURN90  0x5
#define MSG_TURN120 0x6
#define MSG_ACKq    0x7

//Constants
#define BUFFER_SIZE             (256)
#define EXPORT_FILE             "/sys/class/gpio/export"
/************************/

//open GPIO and set the direction
//returns pointer to string containing the gpio pin directory if successful
//  this needs to get freed after you're done with it
//returns null ptr on error
char* openGPIO(int gpio_handle, int direction )
{
    int n;
    //   1. export GPIO
    FILE* exportFh = fopen(EXPORT_FILE, "w");
    if(exportFh== NULL)
    {
        printf("Couldn't not open export file\n");
        return NULL;
    }
    char numBuffer[5];
    snprintf(numBuffer, 5, "%d", gpio_handle);
    n = fputs(numBuffer, exportFh);
    if(n < 0)
    {
        printf("error writing to export file\n");
        return NULL;
    }
    fclose(exportFh);

    //form the file name of the newly created gpio directory
    char *gpioDirectory = malloc(BUFFER_SIZE);

    n = snprintf(gpioDirectory, BUFFER_SIZE, "/sys/class/gpio/gpio%d/", gpio_handle);
    if(n >= BUFFER_SIZE)
    {
        printf("Buffer overflow when creating directory name\n");
        free(gpioDirectory);
        return NULL;
    }

    //    2.set the direction
    char gpioDirection[BUFFER_SIZE];
    strcpy(gpioDirection, gpioDirectory);
    strcat(gpioDirection, "direction");
    FILE* directionFh = fopen(gpioDirection, "w");

    if(directionFh == NULL)
    {
        printf("gpio direction file is null %s \n", gpioDirection);
        free(gpioDirectory);
        return NULL;
    }

    if(direction == GPIO_DIRECTION_OUT)
    {
        n = fputs("out", directionFh);
    }
    else if(direction == GPIO_DIRECTION_IN)
    {
        n = fputs("in", directionFh);
    }
    if(n < 0)
    {
        printf("Error writing to gpio direction file\n");
        free(gpioDirectory);
        return NULL;
    }
    fclose(directionFh);

    //    3.set the votage
    char gpioValue[BUFFER_SIZE];
    strcpy(gpioValue, gpioDirectory);
    strcat(gpioValue, "value");
    FILE* valueFh = fopen(gpioValue, "w");
    n = fputs("1", valueFh);
    if(n < 0)
    {
        printf("Error writing to gpio value file\n");
        free(gpioDirectory);
        return NULL;
    }

    //return the new gpio directory
    return gpioDirectory;
}

//write value (HIGH or LOW) to port specified
//returns value written on success
//returns ERROR (negative) on failure
int writeGPIO(char* gpioDirectory, int value)
{
    char gpioValue[BUFFER_SIZE];
    strcpy(gpioValue, gpioDirectory);
    strcat(gpioValue, "value");
    FILE* valueFh = fopen(gpioValue, "w");

    char numBuffer[5];
    snprintf(numBuffer, 5, "%d", value);

    int n = fputs(numBuffer, valueFh);
    if(n < 0)
    {
        printf("Error writin to gpio value file in %s", gpioDirectory);
        return ERROR;
    }

    return value;
}

//Sets the GPIO pin specified to a new direction
//returns the direction set on success
//returns ERROR(negative) on failure
int setGPIODirection(char* gpioDirectory, int direction)
{
    char gpioDirection[BUFFER_SIZE];
    strcpy(gpioDirection, gpioDirectory);
    strcat(gpioDirection, "direction");
    FILE* directionFh = fopen(gpioDirection, "w");

    int n = -1;
    if(direction == GPIO_DIRECTION_IN)
    {
        n = fputs("in", gpioDirection);
    }
    else if(direction == GPIO_DIRECTION_OUT)
    {
        n = fputs("out", gpioDirection);
    }

    if(n < 0)
    {
        printf("Error writin to gpio value file in %s", gpioDirectory);
        return ERROR;
    }

    return direction;
}

//read value (HIGH or LOW) from port specified
//port direction must be set to input.
//returns port value on success
//returns ERROR (negative) on failure
int readGPIO(char* gpioDirectory)
{
    char gpioValue[BUFFER_SIZE];
    strcpy(gpioValue, gpioDirectory);
    strcat(gpioValue, "value");
    FILE* valueFh = fopen(gpioValue, "r");

    char numBuffer[5];
    char* test = fgets(numBuffer, 5, valueFh);
    if(test == NULL)
    {
        printf("Error reading from gpio value %s", gpioDirectory);
        return ERROR;
    }

    return atoi(numBuffer);
}

//Sends a nibble(4 bytes) along the bus following the Bus Protocol.
//does not wait for an ACK.
void writeNibble(unsigned char data,
    char* d0,
    char* d1,
    char* d2,
    char* d3,
    char* strobe)
{
    //set all the ports to output
    setGPIODirection(d0, GPIO_DIRECTION_OUT);
    setGPIODirection(d1, GPIO_DIRECTION_OUT);
    setGPIODirection(d2, GPIO_DIRECTION_OUT);
    setGPIODirection(d3, GPIO_DIRECTION_OUT);
    setGPIODirection(strobe, GPIO_DIRECTION_OUT);

    //start the bus protocol
    //1: pull strobe low
    gpioValue(strobe, LOW);

    //2: output the nibble to the bus
    writeGPIO(d0, data & 0);
    writeGPIO(d1, (data & 1) >> 1);
    writeGPIO(d2, (data & 2) >> 2);
    writeGPIO(d3, (data & 4) >> 3);

    //3: raise strobe and wait at least 10ms
    writeGPIO(strobe, HIGH);
    usleep(10);

    //4: Pull strobe low again
    writeGPIO(strobe, LOW);

    //5: clear the bus
    writeGPIO(d0, LOW);
    writeGPIO(d1, LOW);
    writeGPIO(d2, LOW);
    writeGPIO(d3, LOW);
}

//Reads a 4 bit nibble from the bus following the protocol
//returns the nibble in the lower 4 bits of the return value
//returns a negative on error
int readNibble(char* d0,
    char* d1,
    char* d2,
    char* d3,
    char* strobe)
{
    unsigned char data = 0x00;
    //set all the data ports to input, but the strobe to output
    setGPIODirection(d0, GPIO_DIRECTION_IN);
    setGPIODirection(d1, GPIO_DIRECTION_IN);
    setGPIODirection(d2, GPIO_DIRECTION_IN);
    setGPIODirection(d3, GPIO_DIRECTION_IN);
    setGPIODirection(strobe, GPIO_DIRECTION_OUT);

    //start the bus protocol
    //1: pull strobe low to signal the start of the read
    gpioValue(strobe, LOW);

    //2: the PIC should output to the bus now. We give it 10ms
    usleep(10);

    //3: raise strobe and start reading the value from the data bus
    writeGPIO(strobe, HIGH);
    data += readGPIO(d0);
    data += readGPIO(d1) << 1;
    data += readGPIO(d2) << 2;
    data += readGPIO(d3) << 3;

    if(data > 0xF)
    {
        printf("Error reading nibble from the bus")
        return ERROR;
    }
    //4: Pull strobe low again to signal that data has been read
    writeGPIO(strobe, LOW);

    //5: the PIC will clear the bus

    return (int)data;
}

// tests the GPIO write and exits
void testGPIO(char * fh)
{
    //test read and write.
    int w = writeGPIO(fh, HIGH);
    assert(w == HIGH && "Write high");
    w = writeGPIO(fh, LOW);
    assert(w == LOW && "Write Low");
    exit(0);
}

//Functions definitions - for commands
void reset();
void ping();
void adc_value();
void servo_30();
void servo_90();
void servo_120();

//main
int main(void)
{
    char* fileHandleGPIO_4;
    char* fileHandleGPIO_5;
    char* fileHandleGPIO_6;
    char* fileHandleGPIO_7;
    char* fileHandleGPIO_S;  //Should these 5 variables be used globally? - (I think they're fine here - AM)

    fileHandleGPIO_4 = openGPIO(GP_4, GPIO_DIRECTION_OUT);
    fileHandleGPIO_5 = openGPIO(GP_5, GPIO_DIRECTION_OUT);
    fileHandleGPIO_6 = openGPIO(GP_6, GPIO_DIRECTION_OUT);
    fileHandleGPIO_7 = openGPIO(GP_7, GPIO_DIRECTION_OUT);
    fileHandleGPIO_S = openGPIO(Strobe, GPIO_DIRECTION_OUT);
    /*Line 92 sets the strobe to LOW as shown in figure 1 of the lab manual. 
    The data lines are also set LOW (lines 99-102) to assure that
    they aren't sending any data */ 
    
    while(1)
    {

    writeGPIO(fileHandleGPIO_S, HIGH); 
    /*For line 106, I'm not sure what the best way to handle 
    the write to of the GPIO is since a #define directive was 
    used (lines 28 & 29 ) to pick high or low. Can you send a
    #define variable and compare it to a normal variable? Even 
    if you can do that, how to physically change direction?
    
    2.write data... (where the menu selection is used? and a bunch of 
    other stuff?))*/

        int input;
        do{
            printf("Select a number for desired action: \n\n");
            printf("1. Reset\n");
            printf("2. Ping\n");
            printf("3. Get ADC value");
            printf("4. Turn Servo 30 degrees\n");
            printf("5. Turn Servo 90 degrees\n");
            printf("6. Turn Servo 120 degrees\n");
            printf("7. Exit\n");

            scanf("%s", input);

            switch (input)
            {
                case '1' :
                reset();
                break;
                case '2'  :
                ping();
                break;
                case '3'  :
                adc_value();
                break;
                case '4'  :
                servo_30();
                break;
                case '5'  :
                servo_90();
                break;
                case '6'  :
                servo_120();
                default :
                printf("Please enter a valid number (1 - 6)\n");
                break;
            }
        }while(1); /// probably incorrect

           //3.Strobe low because Galileo is done writing to the bus
        writeGPIO(fileHandleGPIO_S, LOW);
           //4.Strobe high
        writeGPIO(fileHandleGPIO_S, HIGH);
    }
}