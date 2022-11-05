#include <Arduino.h>

#include "myFlash.hpp"

#define GPIO_KEY 23

#define STRING_BUFFER_SIZE 100
static String str("", STRING_BUFFER_SIZE);


void setup() {
	
	pinMode(GPIO_KEY, INPUT_PULLDOWN);
	
	Serial.begin(115200);
	sleep_ms(5000);

	Serial.println();
	str = "Empty";
	str += " - Setup done";
	Serial.println(str);
	Serial.println();
	
	setupFlash();

}


// --------------------------------------

void loop() {

	// str = "loop";
	// Serial.println(str);
	
	// static uint16_t round = 0;

	while (true) {

		for (auto i = 0u; i < FLASH_PAGES_PER_SECTOR / 2; ++i) {
			sleep_ms(500);
			
			updateFlashBuffer();

			updateFlashData();
			readFlashData();
			printFlashBuffer();
		}


		// waiting to restart - by button KEY
		while (true) {
			sleep_ms(50);

			if (digitalRead(GPIO_KEY)) {
				Serial.println("KEY button pressed");
				sleep_ms(100);
				while (digitalRead(GPIO_KEY)) {
					Serial.println("please release the KEY button");
					sleep_ms(2000);
				}
				
				findCurrentPage ();
				break;
			}

		}
	}	
}

