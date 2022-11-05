// ------------------------------------------ info ------------------------------------------
// good explanation of rp2040 flash
// https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/

// PICO_FLASH_SIZE_BYTES # The total size of the RP2040 flash, in bytes
// FLASH_SECTOR_SIZE     # The size of one sector, in bytes (the minimum amount you can erase)
// FLASH_PAGE_SIZE       # The size of one page, in bytes (the mimimum amount you can write)

// PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE

// ------------------------------- write -----------------------------------------------------
// int buf[FLASH_PAGE_SIZE/sizeof(int)];  // One page worth of 32-bit ints
// int mydata = 123456;  // The data I want to store
// buf[0] = mydata;  // Put the data into the first four bytes of buf[]
// // Erase the last sector of the flash
// flash_range_erase((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
// // Program buf[] into the first page of this sector
// flash_range_program((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), (uint8_t *)buf, FLASH_PAGE_SIZE);

// ------------------------------- read -----------------------------------------------------
// // Flash-based address of the last sector
// #define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
// int *p, addr, value;
// // Compute the memory-mapped address, remembering to include the offset for RAM
// addr = XIP_BASE +  FLASH_TARGET_OFFSET
//     p = (int *)addr; // Place an int pointer at our memory-mapped address
//     value = *p; // Store the value at this address into a variable for later use

// -----------------------------------------------------------------------------------------

extern "C" {
    #include <hardware/sync.h>
    #include <hardware/flash.h>
    #include <limits.h>
};

#define FLASH_TARGET_OFFSET     (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define FLASH_PAGES_PER_SECTOR  (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)
static int addr;
// static int *p, value;

static uint8_t current_page = -1;

const static int buf_num_of_data_var = (FLASH_PAGE_SIZE / sizeof(int));
const static int buf_num_data_used = 5;
const static uint8_t buf_type_size = sizeof(int);
static int buf_tx[buf_num_of_data_var];
static int buf_rx[buf_num_of_data_var];
#include <bitset>

#define STRING_BUFFER_SEPARATION_SIZE 5
const uint8_t STRING_BUFFER_SIZE = (2 * (CHAR_BIT * buf_type_size) + STRING_BUFFER_SEPARATION_SIZE);
static String str_tx_rx("", STRING_BUFFER_SIZE);
static std::bitset<buf_type_size * CHAR_BIT> tx_value;
static std::bitset<buf_type_size * CHAR_BIT> rx_value;


// ----------------------------------------------------------------

void setupFlash ();
void readFlashData ();
void readFlashSectorData ();
void updateFlashData ();
void eraseFlashData ();
void resetFlashBuffer ();
void updateFlashBuffer ();
void updateFlashBuffer (uint8_t val);
void findCurrentPage ();
void printFlashBuffer ();

// ----------------------------------------------------------------

void setupFlash () {
    
    tx_value.reset();
    rx_value.reset();
    resetFlashBuffer();

    for (auto& val : str_tx_rx)
        val = ' ';
    str_tx_rx[str_tx_rx.length() / 2] = '-';
    str_tx_rx[str_tx_rx.length() - 1] = '\0';


    // set read flash essential
    addr = XIP_BASE +  FLASH_TARGET_OFFSET;     // Compute the memory-mapped address, remembering to include the offset for RAM
    // p = (int *)addr;                            // Place an int pointer at our memory-mapped address
    // value = *p;                                 // Store the value at this address into a variable for later use
    
    findCurrentPage();
    readFlashData();
    printFlashBuffer();

}

void readFlashData () {
    
    // read and save page data
    int* pointer_flash_value = (int *)addr;
    for (auto pos = 0; pos < buf_num_data_used; ++pos, ++pointer_flash_value) {
        buf_rx[pos] = *pointer_flash_value;
    }

}

void readFlashSectorData () {
    int page_addr;
    int* pointer_flash_value = nullptr;
    // int value;
    
    static int value_arr[FLASH_PAGES_PER_SECTOR * buf_num_data_used];
    int j = 0;

    // reseting all variables
    for (auto& val : value_arr)
        val = 0;
    j = 0;

    sleep_ms(100);
    uint32_t ints = save_and_disable_interrupts();

    for (auto page = 0; page < FLASH_PAGES_PER_SECTOR; ++page) {
        page_addr = XIP_BASE +  FLASH_TARGET_OFFSET + (page * FLASH_PAGE_SIZE);
        // sleep_ms(100);
        
        // Serial.print("page ");
        // Serial.println(page);

        pointer_flash_value = (int*)page_addr;
        for (auto pos = 0; pos < buf_num_data_used; ++j, ++pos, ++pointer_flash_value) {

            value_arr[j] = *pointer_flash_value;
            // Serial.print(*pointer_flash_value);
            // Serial.print(" ");

            // sleep_ms(500);
            // ++j;
            // sleep_ms(100);
            // value = *pointer_flash_value;
            // Serial.print(pos);
            // Serial.print(".   ");
            // Serial.println(value);
            // sleep_ms(100);
        }
        // sleep_ms(500);
    }

    restore_interrupts (ints);

    uint8_t counter = 0;
    for (auto& i : value_arr) {
        Serial.print(i);
        Serial.print(" ");
        // sleep_ms(50);

        ++counter;
        if (counter == 5) {
            Serial.println();
            counter = 0;
        }

        // sleep_ms(100);
    }

}

void updateFlashData () {
    
    uint32_t ints = save_and_disable_interrupts();
    
    // updating previous page - as done
    buf_tx[0] = 1;
    int previous_addr = FLASH_TARGET_OFFSET + (current_page * FLASH_PAGE_SIZE);
    flash_range_program(previous_addr, (uint8_t *)buf_tx, FLASH_PAGE_SIZE);
    
    // updating current page info
    buf_tx[0] = -1;
    ++current_page;
    if (current_page >= FLASH_PAGES_PER_SECTOR) {
        eraseFlashData();
        current_page = 0;
    }
    addr = XIP_BASE +  FLASH_TARGET_OFFSET + (current_page * FLASH_PAGE_SIZE);
    
    // updating new page with data
    flash_range_program(FLASH_TARGET_OFFSET + (current_page * FLASH_PAGE_SIZE), (uint8_t *)buf_tx, FLASH_PAGE_SIZE);
    restore_interrupts (ints);

    Serial.print("current_page = ");
    Serial.println(current_page);
    
}

void eraseFlashData () {
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
}

void resetFlashBuffer () {
    for (auto& val : buf_tx) {
        val = -1;
    }
    for (auto& val : buf_rx) {
        val = -1;
    }

    // for (auto pos = 0; pos < buf_num_of_data_var; ++pos) {
    //     buf_tx[pos] = -1;
    //     buf_rx[pos] = -1;
    // }
}

void updateFlashBuffer () {
    static int max_val = 0;
    
    resetFlashBuffer();
    for (auto pos = 1; pos < buf_num_data_used; ++pos, ++max_val) {
        buf_tx[pos] =  max_val;
    }
}

void updateFlashBuffer (uint8_t val) {
    for (auto pos = 1; pos < buf_num_of_data_var; ++pos) {
        buf_tx[pos] =  val + (current_page * buf_num_data_used + pos);
        buf_tx[pos] = 0xFFFFFFFF ^ (1u << (val * pos));
    }
}

void findCurrentPage () {
    
    readFlashSectorData();

    Serial.println("--------------------------------------------");
    for (auto page = 0; page < FLASH_PAGES_PER_SECTOR; ++page) {
        addr = XIP_BASE +  FLASH_TARGET_OFFSET + (page * FLASH_PAGE_SIZE);

        // 0xFFFFFFFF cast as an int is -1 so this is how we detect an empty page
        int* pointer_flash_value = (int *)addr;
        Serial.print("page = ");
        Serial.print(page);
        Serial.print("   -   ");
        // Serial.print("pointer_flash_value = ");
        Serial.println(*pointer_flash_value);



        if ( *pointer_flash_value == -1 ){
            current_page = page;                // we found our first empty page, remember it
            break;
        }
    }
    Serial.println("--------------------------------------------");

}


void printFlashBuffer () {

    // Serial.println("\t\t tx \t\t - \t\t rx ");
    // for (auto j = 0; j < buf_num_data_used; ++j) {
    //     tx_value = buf_tx[j];
    //     rx_value = buf_rx[j];

    //     for (auto i = 0; i < tx_value.size(); ++i) {
    //         str_tx_rx[i] = '0' + tx_value[i];
    //     }

    //     auto start_point = (str_tx_rx.length() + STRING_BUFFER_SEPARATION_SIZE) / 2;
    //     for (auto i = 0; i < rx_value.size(); ++i) {
    //         str_tx_rx[start_point + i] = '0' + rx_value[i];
    //     }

    //     Serial.print(j);
    //     Serial.print(".  ");
    //     Serial.println(str_tx_rx);
    // }

    Serial.println("    tx  -  rx");
    for (auto j = 0; j < buf_num_data_used; ++j) {
        Serial.print(j);
        Serial.print(".  ");
        Serial.print(buf_tx[j]);
        Serial.print("  -  ");
        Serial.println(buf_rx[j]);
    }
    Serial.println();

}

// ----------------------------------------------------------------



