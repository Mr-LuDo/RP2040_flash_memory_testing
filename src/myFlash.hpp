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
static int rx_page_addr;
static int tx_page_addr;
// static int *p, value;

static uint8_t current_page = NULL;
// static uint8_t current_page = -1;

#define PAGE_STATUS     0
#define STATUS_EMPTY    0xFFFFFFFF
#define STATUS_USED     1

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
void updateCurrentPage(int page_target);
void updatePageAddress();
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
    
    // tx_value.reset();
    // rx_value.reset();
    resetFlashBuffer();


    // for (auto& val : str_tx_rx)
    //     val = ' ';
    // str_tx_rx[str_tx_rx.length() / 2] = '-';
    // str_tx_rx[str_tx_rx.length() - 1] = '\0';


    // set read flash essential
    // rx_page_addr = XIP_BASE +  FLASH_TARGET_OFFSET;     // Compute the memory-mapped address, remembering to include the offset for RAM
    // p = (int *)addr;                            // Place an int pointer at our memory-mapped address
    // value = *p;                                 // Store the value at this address into a variable for later use
    
    findCurrentPage();
    readFlashData();
    printFlashBuffer();

}

// update Current page to specific page or
// update to the next one
void updateCurrentPage(int page_target = -1) {
    if (page_target == -1)
        ++current_page;
    else
        current_page = page_target;

    if (current_page >= FLASH_PAGES_PER_SECTOR) {
        current_page = 0;
        // readFlashSectorData();
        eraseFlashData();
    }
    updatePageAddress();
}

void updatePageAddress() {
    tx_page_addr = FLASH_TARGET_OFFSET + (current_page * FLASH_PAGE_SIZE);
    rx_page_addr = XIP_BASE + tx_page_addr;
}

// read and save page data
void readFlashData () {
    updatePageAddress();
    int* pointer_flash_value = (int *)rx_page_addr;
    for (auto pos = PAGE_STATUS; pos < buf_num_data_used; ++pos, ++pointer_flash_value) {
        buf_rx[pos] = *pointer_flash_value;
    }
}

void readFlashSectorData () {
    int page_addr;
    int* pointer_flash_value = nullptr;
    // int value;
    
    int value_arr[FLASH_PAGES_PER_SECTOR * buf_num_data_used];
    int j = 0;

    // reseting all variables
    for (auto& val : value_arr)
        val = 0;
    j = 0;

    for (auto page = 0u; page < FLASH_PAGES_PER_SECTOR; ++page) {
        page_addr = XIP_BASE +  FLASH_TARGET_OFFSET + (page * FLASH_PAGE_SIZE);
        // sleep_ms(100);
        
        // Serial.print("page ");
        // Serial.println(page);

        pointer_flash_value = (int*)page_addr;
        for (auto pos = 0; pos < buf_num_data_used; ++j, ++pos, ++pointer_flash_value) {

            // value_arr[(page * buf_num_data_used) + pos] = *((int*)rx_pages_addr_arr[page] + pos);
            
            // working kinda
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


    uint8_t counter = 0;
    for (auto& i : value_arr) {
        Serial.print(i);
        Serial.print(" ");

        ++counter;
        if (counter == buf_num_data_used) {
            Serial.println();
            counter = 0;
            // Serial.flush();
        }
    }

}

void updateFlashData () {
    
    updateCurrentPage();
    
    uint32_t ints = save_and_disable_interrupts();
    // updating previous page - as done
    // updatePageAddress();
    // buf_rx[PAGE_STATUS] = 1;
    // flash_range_program(tx_page_addr, (uint8_t *)buf_rx, FLASH_PAGE_SIZE);
    
    // updateCurrentPage();
    
    // updating new page with data
    flash_range_program(tx_page_addr, (uint8_t *)buf_tx, FLASH_PAGE_SIZE);
    restore_interrupts (ints);
   
}

void eraseFlashData () {
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
}

void resetFlashBuffer () {
    for (auto& val : buf_tx) {
        val = STATUS_EMPTY;
        // val = -1;
    }
    for (auto& val : buf_rx) {
        val = STATUS_EMPTY;
        // val = -1;
    }
}

void updateFlashBuffer () {
    static int max_val = 0;
    
    // resetFlashBuffer();
    buf_tx[PAGE_STATUS] = STATUS_USED;
    for (auto pos = 1; pos < buf_num_data_used; ++pos, ++max_val) {
        buf_tx[pos] =  max_val;
    }
}

void updateFlashBuffer (uint8_t val) {
    buf_tx[PAGE_STATUS] == STATUS_EMPTY;
    for (auto pos = 1; pos < buf_num_of_data_var; ++pos) {
        buf_tx[pos] =  val + (current_page * buf_num_data_used + pos);
        buf_tx[pos] = STATUS_EMPTY ^ (1u << (val * pos));
    }
}

void findCurrentPage () {
    
    readFlashSectorData();

    int page_satus_addr;
    Serial.println("--------------------------------------------");
    for (auto page = 0; page < FLASH_PAGES_PER_SECTOR; ++page) {
        page_satus_addr = XIP_BASE +  FLASH_TARGET_OFFSET + (page * FLASH_PAGE_SIZE);

        // 0xFFFFFFFF cast as an int is -1 so this is how we detect an empty page
        // int* page_status = (int *)rx_page_addr;
        // Serial.print("page = ");
        // Serial.print(page);
        // Serial.print("   -   ");
        // // Serial.print("page_status = ");
        // Serial.println(*page_status);
        
        // // Serial.print("   -   ");
        // // Serial.println((*page_status) == STATUS_EMPTY? "Current page": " Old data");

        // if (*page_status == STATUS_EMPTY){
        //     updateCurrentPage(page - 1);                // we found our first empty page, remember it
        //     break;
        // }

        bool page_empty = ((*(int *)page_satus_addr) == (int)STATUS_EMPTY)? true: false;
        Serial.print("page ");
        Serial.print(page);
        Serial.print("  -  ");
        Serial.println(page_empty? "Empty": "In use");

        // we found our first empty page
        // the page before the first empty
        // is the current page
        if (page_empty){
            updateCurrentPage(page - 1);                    
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
    
    Serial.print("current_page = ");
    Serial.println(current_page);

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



