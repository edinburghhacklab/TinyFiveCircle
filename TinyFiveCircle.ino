// Tiny85 versioned - ATmega requires these defines to be redone as well as the
// DDRB/PORTB calls later on.

/* commentary: 

Future modes:

1. random color random position - existing mode

2. hue walk at constant brightness - existing mode

3. brightness walk at constant hue - existing mode

4. saturation walk is kind of obnoxious - existing mode

5. "Rain" - start at LED1 with a random color.  Advance color to LED2 and pick a
   new LED1.  Rotate around the ring.

6. non-equidistant hue walks - instead of 0 72 144 216 288 incrementing,
   something like...  0 30 130 230 330 rotating around the ring.  The hue += and
   -= parts might help here?

7. Color morphing: Pick 5 random colors.  Transition from color on LED1 to LED2
   and around the ring.  Could be seen as a random variant of #2.

8. Larson scanner in primaries - Doable with 5 LEDs? (probably, I've seen
   larsons with 2 actively lit LEDs in 4 LEDs before.)

9. Pick a color and sweep brightness around the circle (red! 0-100
   larson-style), then green 0-100, the blue 0-100, getting new color on while
   old color is fading out.

*/

// Because I tend to forget - the number here is the Port B pin in use.  Pin 5 is PB0, Pin 6 is PB1, etc.
#define LINE_A 0 // PB0 / Pin 5 on ATtiny85 / Pin 14 on an ATmega328 (D8)
#define LINE_B 1 // PB1 / Pin 6 on ATtiny85 / Pin 15 on an ATmega328 (D9) 
#define LINE_C 2 // PB2 / Pin 7 on ATtiny85 / Pin 17 on an ATmega328 (D11)
#define LINE_D 3 // PB3 / Pin 2 on ATtiny85 / Pin 18 on an ATmega328 (D12)
#define LINE_E 4 // PB4 / Pin 3 on ATtiny85

#include <avr/sleep.h>
#include <avr/power.h>
#include <EEPROM.h>

// How many modes do we want to go through?
#define MAX_MODE 9
// How long should I draw each color on each LED?
#define DRAW_TIME 5

// Location of the brown-out disable switch in the MCU Control Register (MCUCR)
#define BODS 7 
// Location of the Brown-out disable switch enable bit in the MCU Control Register (MCUCR)
#define BODSE 2 

// The base unit for the time comparison. 1000=1s, 10000=10s, 60000=1m, etc.
//#define time_multiplier 1000 
#define time_multiplier 60000
// How many time_multiplier intervals should run before going to sleep?
#define run_time 240

#define MAX_HUE 360 // normalized

// How bit-crushed do you want the bit depth to be?  1 << TIMESCALE is how
// quickly it goes through the LED softpwm scale.

// 1 means 128 shades per color.  3 is 32 colors while 6 is 4 shades per color.
// it sort of scales the time drawing routine, but not well.

// It also affects how bright it it (less time spent drawing nothing) as well as
// the POV flickering rate. Higher numbers are both brighter and less flicker-y.

// the build uses 1 for the PTH versions and 3 for the SMD displays.

#define TIMESCALE 1

byte __attribute__ ((section (".noinit"))) last_mode;

uint8_t led_grid[15] = {
  000 , 000 , 000 , 000 , 000 , // R
  000 , 000 , 000 , 000 , 000 , // G
  000 , 000 , 000 , 000 , 000  // B
};

uint8_t led_grid_next[15] = {
  000 , 000 , 000 , 000 , 000 , // R
  000 , 000 , 000 , 000 , 000 , // G
  000 , 000 , 000 , 000 , 000  // B
};


void setup() {
  if(bit_is_set(MCUSR, PORF)) { // Power was just established!
    MCUSR = 0; // clear MCUSR
    EEReadSettings(); // read the last mode out of eeprom
  } 
  else if(bit_is_set(MCUSR, EXTRF)) { // we're coming out of a reset condition.
    MCUSR = 0; // clear MCUSR
    last_mode++; // advance mode

    if(last_mode > MAX_MODE) {
      last_mode = 0; // reset mode
    }
  }

  // Try and set the random seed more randomly.  Alternate solutions involve
  // using the eeprom and writing the last seed there.
  uint16_t seed=0;
  uint8_t count=32;
  while (--count) {
    seed = (seed<<1) | (analogRead(1)&1);
  }
  randomSeed(seed);
}

void loop() {
  // indicate which mode we're entering
  led_grid[last_mode] = 255;
  draw_for_time(1000);
  led_grid[last_mode] = 0;
  delay(250);

  // If EXTRF hasn't been asserted yet, save the mode
  EESaveSettings();

  // go into the modes
  switch(last_mode) {
    // red modes
  case 0: 
    HueWalk(run_time,millis(),20,1); // wide virtual space, slow progression
    break;
  case 1:
    HueWalk(run_time,millis(),20,2); // wide virtual space, medium progression
    break;
  case 2:
    HueWalk(run_time,millis(),20,5); // wide virtual space, fast progression
    break;
  case 3:
    HueWalk(run_time,millis(),5,1); // 1:1 space to LED, slow progression
    break;
  case 4:
    HueWalk(run_time,millis(),5,2); // 1:1 space to LED, fast progression
    break;
    // green modes
  case 5: 
    RandomColorRandomPosition(run_time,millis());
    break;
  case 6:
    SBWalk(run_time,millis(),1,1); // Slow progression through hues modifying brightness
    break; 
  case 7:
    SBWalk(run_time,millis(),4,1); // fast progression through hues modifying brightness
    break;
  case 8:
    PrimaryColors(run_time,millis());
    break;
  case 9:
    AllRand();
    break;
  }
  SleepNow();
}

void AllRand(void) {
  uint8_t allrand_time = 5;
  while(1) {
    for(int x = 0; x<=15; x++) {
      led_grid[x] = 0;
    }
    
    int randmode = random(8);
    
    switch(randmode) {
      // red modes
    case 0: 
      HueWalk(allrand_time,millis(),20,1); // wide virtual space, slow progression
      break;
    case 1:
      HueWalk(allrand_time,millis(),20,2); // wide virtual space, medium progression
      break;
    case 2:
      HueWalk(allrand_time,millis(),20,5); // wide virtual space, fast progression
      break;
    case 3:
      HueWalk(allrand_time,millis(),5,1); // 1:1 space to LED, slow progression
      break;
    case 4:
      HueWalk(allrand_time,millis(),5,2); // 1:1 space to LED, fast progression
      break;
      // green modes
    case 5: 
      RandomColorRandomPosition(allrand_time,millis());
      break;
    case 6:
      SBWalk(allrand_time,millis(),1,1); // Slow progression through hues modifying brightness
      break; 
    case 7:
      SBWalk(allrand_time,millis(),4,1); // fast progression through hues modifying brightness
      break;
    case 8:
      PrimaryColors(allrand_time,millis());
      break;
      
    }
  }
}
void SleepNow(void) {
  // decrement last_mode, so the EXTRF increment sets it back to where it was.
  // note: this actually doesn't work that great in cases where power is removed after the MCU goes to sleep.
  // On the other hand, that's an edge condition that I'm not going to worry about too much.
  last_mode--;

  // mode is type byte, so when it rolls below 0, it will become a Very
  // Large Number compared to MAX_MODE.  Set it to MAX_MODE and the setup
  // routine will jump it up and down by one.
  if(last_mode > MAX_MODE) { last_mode = MAX_MODE; }

  EESaveSettings();

  // Important power management stuff follows
  ADCSRA &= ~(1<<ADEN); // turn off the ADC
  ACSR |= _BV(ACD);     // disable the analog comparator
  MCUCR |= _BV(BODS) | _BV(BODSE); // turn off the brown-out detector

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // do a complete power down
  sleep_enable(); // enable sleep mode
  sei();  // allow interrupts to end sleep mode
  sleep_cpu(); // go to sleep
  delay(500);
  sleep_disable(); // disable sleep mode for safety
}

/* currently funky.

void LarsonScanner(uint16_t time, uint32_t start_time, bool sleep) {
  int8_t direction = 1;
  uint8_t active = 1;
  uint8_t width = 5;
  int array[width];
  int mode = 1;

  // blank out the array...
  for(uint8_t x = 0; x<width; x++) {
    array[x] = 0;
  }

  uint16_t hue = random(MAX_HUE);

  while(1) {
    if(millis() >= (start_time + (time * time_multiplier))) { 
      if(sleep) {
	SleepNow();
      }
      else {
	break;
      }
    }

    // change which member is the active member
    if(direction == 1) { active++; }
    else               { active--; }

    if(mode == 1) {
      if(active > (width-1)) {
	active = 0;
	hue++;
	if(hue >= MAX_HUE) 
	  hue = 0;
      }
    }
    // If the active element will go outside of the realm of the array
    else if ( mode == 0 ) {
      if((active >= (width - 1)) // high side match
	 || (active < 1 )) {  // low side match
	if(direction == 1)       { direction = -1; }
	else if(direction == -1) { direction = 1; }
      }
    }

    // pin the currently active member to the maximum brightness
    array[active] = 255;

    // dim the other members of the array

    // start 1 element off the current.  If I start at 0, I would be
    // dimming the currently active element.
    for(uint8_t x = 0; x<width; x++) {
      // constrain the edits to real array elements.
      if(x != active) {
	array[x] -= (255/(width)+25);
      }
      if(array[x] < 0) { array[x] = 0; }
    }

    uint8_t displaypos = 0;
    for(uint8_t x = 0; x < width; x++) {
      setLedColorHSV(displaypos, hue, 255, array[x]);
      displaypos++;
    }
    fade_to_next_frame();
  }
  return;
}
*/
  
void RandomColorRandomPosition(uint16_t time, uint32_t start_time) {
  // preload all the LEDs with a color
  for(int x = 0; x<5; x++) {
    setLedColorHSV(x, random(MAX_HUE), 255, 255);
  }
  // and start blinking new ones in once a second.
  while(1) {
    setLedColorHSV(random(5), random(MAX_HUE), 255, 255);
    draw_for_time(1000);
    if(millis() >= (start_time + (time * time_multiplier))) { break; }
  }
}

void HueWalk(uint16_t time, uint32_t start_time, uint8_t width, uint8_t speed) {
  while(1) {
    
    if(millis() >= (start_time + (time * time_multiplier))) { break; }

    for(int16_t colorshift=MAX_HUE; colorshift>0; colorshift = colorshift - speed) {

      if(millis() >= (start_time + (time * time_multiplier))) { break; }
      for(uint8_t led = 0; led<5; led++) {
	uint16_t hue = ((led) * MAX_HUE/(width)+colorshift)%MAX_HUE;
	setLedColorHSV(led, hue, 255, 255);
	draw_for_time(DRAW_TIME);
      }
    }
  }
}

/*
time: How long it should run
jump: How much should hue increment every time an LED flips direction?
mode: 
  1 = walk through brightnesses at full saturation, 
  2 = walk through saturations at full brightness.
1 tends towards colors & black, 2 tends towards colors & white.
note: mode 2 is kind of funny looking with the current HSV->RGB code.  
*/
void SBWalk(uint16_t time, uint32_t start_time, uint8_t jump, uint8_t mode) {
  uint8_t scale_max, delta;
  uint16_t hue = random(MAX_HUE); // initial color
  uint8_t led_val[5] = {37,29,21,13,5}; // some initial distances
  bool led_dir[5] = {1,1,1,1,1}; // everything is initially going towards higher brightnesses

  scale_max = 254; 
  delta = 2; 

  while(1) {
    if(millis() >= (start_time + (time * time_multiplier))) { break; }
    for(uint8_t led = 0; led<5; led++) {
      if(mode == 1) {
	setLedColorHSV(led, hue, 254,          led_val[led]);
      } 
      else if (mode == 2) {
	setLedColorHSV(led, hue, led_val[led], 254);
      }
      draw_for_time(2);
      
      // if the current value for the current LED is about to exceed the top or the bottom, invert that LED's direction
      if((led_val[led] >= (scale_max-delta)) or (led_val[led] <= (0+delta))) {
	led_dir[led] = !led_dir[led];
	if(led_val[led] <= (0+delta)) {
	  hue += jump;
	}
	if(hue >= MAX_HUE)
	  hue = 0;
      }
      if(led_dir[led] == 1)
        led_val[led] += delta;
      else 
        led_val[led] -= delta;
    }
  }
}

void PrimaryColors(uint16_t time, uint32_t start_time) {
  uint8_t led_bright = 1;
  bool led_dir = 1;
  uint8_t led = 0;
  while(1) {
    if(millis() >= start_time + (time*time_multiplier)) { break; }
    
    // flip the direction when the LED is at full brightness or no brightness.
    if((led_bright >= 255) or (led_bright <= 0))
      led_dir = !led_dir;
    
    // increment or decrement the brightness
    if(led_dir == 1)
      led_bright++;
    else
      led_bright--;
    
    // if the decrement will turn off the current LED, switch to the next LED
    if( led_bright <= 0 ) { 
      led_grid[led] = 0; 
      led++; 
    }
    
    // And if that change pushes the current LED off the end of the spire, reset to the first LED.
    if( led >=15) 
      led = 0; 

    // push the change out to the array.
    led_grid[led] = led_bright;
    draw_for_time(2);
  }
}

const byte dim_curve[] = {
  0,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,
  3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,
  4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,
  6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,
  8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  11,  11,  11,
  11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  14,  14,  14,  14,  15,
  15,  15,  16,  16,  16,  16,  17,  17,  17,  18,  18,  18,  19,  19,  19,  20,
  20,  20,  21,  21,  22,  22,  22,  23,  23,  24,  24,  25,  25,  25,  26,  26,
  27,  27,  28,  28,  29,  29,  30,  30,  31,  32,  32,  33,  33,  34,  35,  35,
  36,  36,  37,  38,  38,  39,  40,  40,  41,  42,  43,  43,  44,  45,  46,  47,
  48,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
  63,  64,  65,  66,  68,  69,  70,  71,  73,  74,  75,  76,  78,  79,  81,  82,
  83,  85,  86,  88,  90,  91,  93,  94,  96,  98,  99,  101, 103, 105, 107, 109,
  110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
  146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
  193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
};


/*
Inputs:
  p : LED to set
  immediate : Whether the change should go to led_grid or led_grid_next
  hue : 0-360 - color
  sat : 0-255 - how saturated should it be? 0=white, 255=full color
  val : 0-255 - how bright should it be? 0=off, 255=full bright
*/
void setLedColorHSV(uint8_t p, int16_t hue, int16_t sat, int16_t val) {

  /*
    so dim_curve duplicates too many of the low numbers too long, causing the
    animations to look janky.  Maybe 6 2's and 14 3's can be adjusted in other ways.
    
    val = dim_curve[val];
    sat = 255-dim_curve[255-sat];
  */

  while (hue > 360) hue -= 361;
  while (hue < 0) hue += 361;

  int r, g, b, base;

  if (sat == 0) { // Acromatic color (gray). Hue doesn't mind.
    r = g = b = val;
  }
  else {
    base = ((255 - sat) * val)>>8;

    switch(hue/60) {
    case 0:
      r = val;
      g = (((val-base)*hue)/60)+base;
      b = base;
      break;

    case 1:
      r = (((val-base)*(60-(hue%60)))/60)+base;
      g = val;
      b = base;
      break;

    case 2:
      r = base;
      g = val;
      b = (((val-base)*(hue%60))/60)+base;
      break;

    case 3:
      r = base;
      g = (((val-base)*(60-(hue%60)))/60)+base;
      b = val;
      break;

    case 4:
      r = (((val-base)*(hue%60))/60)+base;
      g = base;
      b = val;
      break;

    case 5:
      r = val;
      g = base;
      b = (((val-base)*(60-(hue%60)))/60)+base;
      break;
    }
  }

  // output range is 0-255

  set_led_rgb(p,r,g,b);
}

/* Args:
   position - 0-4
   red value - 0-100
   green value - 0-100
   blue value - 0-100
*/
void set_led_rgb (uint8_t p, uint8_t r, uint8_t g, uint8_t b) {
  led_grid[p] = r;
  led_grid[p+5] = g;
  led_grid[p+10] = b;
}

// runs draw_frame a supplied number of times.
void draw_for_time(uint16_t time) {
  for(uint16_t f = 0; f<time * (1 << (TIMESCALE-1)); f++) { draw_frame(); }
}

// Anode | cathode
const uint8_t led_dir[15] = {
  ( 1<<LINE_B | 1<<LINE_A ), // 4 r
  ( 1<<LINE_A | 1<<LINE_E ), // 5 r
  ( 1<<LINE_E | 1<<LINE_D ), // 1 r
  ( 1<<LINE_D | 1<<LINE_C ), // 2 r
  ( 1<<LINE_C | 1<<LINE_B ), // 3 r

  ( 1<<LINE_B | 1<<LINE_C ), // 4 g
  ( 1<<LINE_A | 1<<LINE_B ), // 5 g
  ( 1<<LINE_E | 1<<LINE_A ), // 1 g
  ( 1<<LINE_D | 1<<LINE_E ), // 2 g
  ( 1<<LINE_C | 1<<LINE_D ), // 3 g

  ( 1<<LINE_B | 1<<LINE_D ), // 4 b
  ( 1<<LINE_A | 1<<LINE_C ), // 5 b
  ( 1<<LINE_E | 1<<LINE_B ), // 1 b
  ( 1<<LINE_D | 1<<LINE_A ), // 2 b
  ( 1<<LINE_C | 1<<LINE_E ), // 3 b
};

//PORTB output config for each LED (1 = High, 0 = Low)
const uint8_t led_out[15] = {
  ( 1<<LINE_B ), // 4 r
  ( 1<<LINE_A ), // 5 r
  ( 1<<LINE_E ), // 1 r
  ( 1<<LINE_D ), // 2 r
  ( 1<<LINE_C ), // 3 r
  
  ( 1<<LINE_B ), // 4 g
  ( 1<<LINE_A ), // 5 g
  ( 1<<LINE_E ), // 1 g
  ( 1<<LINE_D ), // 2 g
  ( 1<<LINE_C ), // 3 g
  
  ( 1<<LINE_B ), // 4 b
  ( 1<<LINE_A ), // 5 b
  ( 1<<LINE_E ), // 1 b
  ( 1<<LINE_D ), // 2 b
  ( 1<<LINE_C ), // 3 b
};

void light_led(uint8_t led_num) { //led_num must be from 0 to 19
  //DDRD is the ports in use on an ATmega328, DDRB on an ATtiny85
  DDRB = led_dir[led_num];
  PORTB = led_out[led_num];
}

void leds_off() {
  DDRB = 0;
  PORTB = 0;
}

void draw_frame(void){
  uint16_t b;
  uint8_t led;
  // giving the loop a bit of breathing room seems to prevent the last LED from flickering.  Probably optimizes into oblivion anyway.
  for ( led=0; led<=15; led++ ) { 
    // software PWM 
    // input range is 0 (off) to 255 (fully on)
    // the b+=8 means it only draws 32 distinct brightnesses instead of the 256 possible otherwise. The affects the refresh rate, which affects the brightness.
    // the upshot is that draw_for_time needs a scaling factor so the animations are constant.


    // Light the LED in proportion to the value in the led_grid array
    for( b=0; b<led_grid[led]; b += 1<<TIMESCALE )
      light_led(led);
    // and turn the LEDs off for the amount of time in the led_grid array between LED brightness and 255.
    for( b=led_grid[led]; b<255; b += 1<<TIMESCALE )
      leds_off();
  }
}

void EEReadSettings (void) {  // TODO: Detect ANY bad values, not just 255.
  byte detectBad = 0;
  byte value = 255;
  
  value = EEPROM.read(0);
  if (value > MAX_MODE)
    detectBad = 1;
  else
    last_mode = value;  // MainBright has maximum possible value of 8.
  
  if (detectBad)
    last_mode = 0; // I prefer the rainbow effect.
}

void EESaveSettings (void){
  //EEPROM.write(Addr, Value);
  
  // Careful if you use  this function: EEPROM has a limited number of write
  // cycles in its life.  Good for human-operated buttons, bad for automation.
  
  byte value = EEPROM.read(0);

  if(value != last_mode)
    EEPROM.write(0, last_mode);
}

