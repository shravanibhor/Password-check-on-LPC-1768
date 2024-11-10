#include <LPC17xx.h>
#include <stdio.h> // For sprintf

unsigned char row, var, flag, key, disp[3];                     // Key for showing pressed Key, Disp for Displaying it with \0
unsigned long int i, j, var1, temp, temp1, temp2, temp3;
unsigned long LED = 0x00000010;     // For LED Initial Setting
unsigned char debugBuffer[20];
unsigned char SCAN_CODE[16] = {0x11, 0x21, 0x41, 0x81,          // Keyboard Initialization
                               0x12, 0x22, 0x42, 0x82,
                               0x14, 0x24, 0x44, 0x84,
                               0x18, 0x28, 0x48, 0x88};

unsigned char ASCII_CODE[16] = {'0', '1', '2', '3',             // Corresponding Buttons for Keyboard
                                '4', '5', '6', '7',
                                '8', '9', '+', '-',
                                '=', 'D', 'E', 'F'};

int password[4] = {1, 2, 3, 4};                                 // Write / Change Password here

// LCD Functions

void Delay_LCD(unsigned int r1) 
{
    unsigned int r;
    for (r = 0; r < r1; r++);
    return;
}

void Clear_Ports(void) 
{
    LPC_GPIO0->FIOCLR = 0x0F << 23;                 // Clearing Data lines
    LPC_GPIO0->FIOCLR = 1 << 27;                    // Clearing RS line
    LPC_GPIO0->FIOCLR = 1 << 28;                    // Clearing Enable line
    return;
}

void Write(int temp2, int type) 
{
    Clear_Ports();
    LPC_GPIO0->FIOPIN = temp2;                      // Assign the value to the data lines
    if (type == 0)
        LPC_GPIO0->FIOCLR = 1 << 27;                // Clear bit RS for Command
    else
        LPC_GPIO0->FIOSET = 1 << 27;                // Set bit RS for Data
    LPC_GPIO0->FIOSET = 1 << 28;                    // Pulse EN Bit
    Delay_LCD(10000);
    LPC_GPIO0->FIOCLR = 1 << 28;                    
    return;
}

void LCD_ComData(int temp1, int type)               // 4 Bit Mode LCD, so necessary shifts required for sending Commands/Data
{
    int temp2 = temp1 & 0xf0;                       
    temp2 = temp2 << 19;                            
    Write(temp2, type);

    temp2 = temp1 & 0x0f;
    temp2 = temp2 << 23;
    Write(temp2, type);

    Delay_LCD(10000);
    return;
}

void LCD_Puts(unsigned char *buf1)                  // Function for displaying on LCD
{
    unsigned int i = 0;
    unsigned int temp3;
    while (buf1[i] != '\0') 
    {
        temp3 = buf1[i];
        LCD_ComData(temp3, 1);
        i++;
        if (i == 16) 
        {
            LCD_ComData(0xC0, 0);                   // Go to next Line
        }
    }
    return;
}

void LCD_Init() 
{
    LPC_PINCON->PINSEL1 &= 0xFC003FFF;                              // P0.23 to P0.28 as GPIO
    LPC_GPIO0->FIODIR |= 0x0F << 23 | 1 << 27 | 1 << 28;            // Setting above bits as Output

    Clear_Ports();
    Delay_LCD(32000);

    // LCD Startup Functions
    LCD_ComData(0x33, 0);               // 4 Bit Mode Initialization
    Delay_LCD(300000);
    LCD_ComData(0x32, 0);
    Delay_LCD(300000);
    LCD_ComData(0x28, 0);               // Function Set
    Delay_LCD(300000);
    LCD_ComData(0x0c, 0);               // Display ON Cursor OFF
    Delay_LCD(80000);
    LCD_ComData(0x06, 0);               // Entry Mode Set, Increment Cursor right
    Delay_LCD(800);
    LCD_ComData(0x01, 0);               // Display Clear
    Delay_LCD(100000);
    return;
}

void scan(void)                         // Checking for Key Press
{
    temp3 = LPC_GPIO1->FIOPIN;
    temp3 &= 0x07800000;                // Check bits 23 to 26 for enabled row
    if (temp3 != 0) 
    {
        flag = 1;
        temp3 >>= 19;                   // Shifted to come to Higher Nibble
        temp >>= 10;                    // Shifted to come to Lower Nibble
        key = temp3 | temp;             // Get SCAN_CODE (0 to 7 bits)
    }
}

int checkPassword(int input[]) 
{
    unsigned int i;
    for (i = 0; i < 4; ++i) 
    {
        if (input[i] != password[i])        // If any digit not matching,
            return 0;                       // Incorrect password, return False
    }
    return 1;                               // Else Correct password, return True
}

void LEDSuccess(void)                       // LED Must Serially Turn ON and OFF for showing Successful Password Attempt
{
    LPC_PINCON->PINSEL0 &= 0xFF0000FF;      // GPIO and Output Initialization
    LPC_GPIO0->FIODIR |= 0x00000FF0;
    while(1)
    {
        LED = 0x00000010; 
        for(i = 1; i < 9; i++)                    // ON LEDs serially (8 times)
        {
            LPC_GPIO0->FIOSET = LED;        // Set LED at LSB as High (on)
            for(j = 0; j < 1000; j++);
            LED <<= 1;                      // Shift to next LED
        }
        LED = 0x00000010;
        for(i = 1; i < 9; i++)                    // OFF LEDs serially
        {
            LPC_GPIO0->FIOCLR = LED;        // Set LED at LSB as Low (off)
            for(j = 0; j < 5000; j++);
            LED <<= 1;                      // Shift to next LED
        }
    }
}

void LEDFail(void) // LED Should Not Glow for Unsuccessful Password Attempt
{
    // No LED activity; just return.
    return;
}

int main(void)                              
{
    int inputPassword[4] = {0};              // Clear input array at start
    int passwordIndex = 0;                   // For checking index

    LPC_GPIO2->FIODIR |= 0x3C00;             // Make Output P2.10 to P2.13 (Rows)
    LPC_GPIO1->FIODIR &= 0xF87FFFFF;         // Make Input P1.23 to P1.26 (Cols)

    LPC_GPIO0->FIODIR |= 0x0F << 23 | 1 << 27 | 1 << 28; // LCD Initialization

    Clear_Ports();
    Delay_LCD(32000);
    
    LCD_Init();

    LCD_ComData(0x80, 0);                   // Point to First Line of LCD
    Delay_LCD(80000);
    LCD_Puts("Enter Password:");            // Print Enter Password in first line

    LCD_ComData(0xC0, 0);                    // Point to Next Line of LCD
    Delay_LCD(800);
    
    while (1) 
    {
        while (passwordIndex < 4) 
        {
            for (row = 1; row < 5; row++) 
            {
                if (row == 1)
                    var1 = 1 << 10;
                else if (row == 2)
                    var1 = 1 << 11;
                else if (row == 3)
                    var1 = 1 << 12;
                else if (row == 4)
                    var1 = 1 << 13;
                temp = var1;
                LPC_GPIO2->FIOCLR = 0x3C00;         // Clear Port and send Value for Enabling Row
                LPC_GPIO2->FIOSET = var1;           // Enabling the Row
                flag = 0;
                scan();                             // Check if any key pressed in Enabled Row
                if (flag == 1)                      // If Pressed
                {
                    break;
                }
            } 
            if (flag == 1) 
            {
                for (i = 0; i < 16; i++)            // Get Corresponding Letter / Digit to Display
                {
                    if (key == SCAN_CODE[i]) 
                    {
                        key = ASCII_CODE[i];
                        // Store only if the key is a valid digit
                        if (key >= '0' && key <= '9' && passwordIndex < 4) {
                            inputPassword[passwordIndex++] = key - '0'; // Store as integer
                            
                            // Clear the LCD and show current input
                            LCD_ComData(0x01, 0); // Clear Display
                            Delay_LCD(80000);
                            
                            // Display the entered digit
                            disp[0] = key;
                            disp[1] = '\0'; // Null-terminate for display
                            LCD_Puts(disp); // Displaying Pressed Key
                            Delay_LCD(200000); // Adjust this value for longer display
                            
                            // Display current input for debugging
                            sprintf(debugBuffer, "Input: %d%d%d%d", inputPassword[0], inputPassword[1], inputPassword[2], inputPassword[3]);
                            LCD_ComData(0xC0, 0); // Move to second line
                            LCD_Puts(debugBuffer); // Display current input
                            Delay_LCD(50000000); // Longer delay for user to see the input
                        }
                        break;
                    }
                }
            }
        } 

        LCD_ComData(0x01, 0);                       // Clear Display
        Delay_LCD(80000);
        
        if (checkPassword(inputPassword))           // Check with Defined Password and print required Message and LED Pattern
        {
            LCD_Puts("Access Granted");
            LEDSuccess();
        } 
        else 
        {
            LCD_Puts("Access Denied");
            LEDFail();
        }
        
        passwordIndex = 0;			                // Reset Variable for Next attempt
        for (i = 0; i < 4; i++) {
            inputPassword[i] = 0; // Clear input array for the next attempt
        }
    } 
}
