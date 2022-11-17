// ------------------------------------------ info ------------------------------------------
// good explanation for rp2040 flash
// https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/

// PICO_FLASH_SIZE_BYTES # The total size of the RP2040 flash, in bytes                 - 2 MB  - 2048 KB   - 2,097,152 Bytes
// FLASH_SECTOR_SIZE     # The size of one sector, in bytes (the minimum amount you can erase)  - 4 KB      - 4,096 Bytes
// FLASH_PAGE_SIZE       # The size of one page, in bytes (the mimimum amount you can write)    - 1/4 KB    - 256 Bytes
// there are no block    # a block is several sectors with is the minimum that can be accessed -  each byte can be accessed

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

#define FLASH_LAST_SECTOR_ADDRESS_FOR_TX   (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define FLASH_LAST_SECTOR_ADDRESS_FOR_RX   (XIP_BASE + FLASH_LAST_SECTOR_ADDRESS_FOR_TX)
#define FLASH_NUM_PAGES_PER_SECTOR         (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)


static int rx_page_addr;
static int tx_page_addr;
// static int *p, value;

static uint8_t current_page = NULL;

#define PAGE_STATUS_POS         0
#define PAGE_STATUS_EMPTY       0xFFFFFFFF
#define PAGE_STATUS_USED        1

const static int buf_num_of_data_var = (FLASH_PAGE_SIZE / sizeof(int));
const static int buf_num_data_used = 5;
const static uint8_t buf_type_size = sizeof(int);
static int buf_tx[buf_num_of_data_var];
static int buf_rx[buf_num_of_data_var];

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
    
    resetFlashBuffer();

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

    if (current_page >= FLASH_NUM_PAGES_PER_SECTOR) {
        current_page = 0;
        // readFlashSectorData();
        eraseFlashData();
    }
    updatePageAddress();
}

void updatePageAddress() {
    // tx_page_addr = FLASH_TARGET_LAST_SECTOR_IN_FLASH + (current_page * FLASH_PAGE_SIZE);
    // rx_page_addr = XIP_BASE + tx_page_addr;

    int page_offset = current_page * FLASH_PAGE_SIZE;
    tx_page_addr = FLASH_LAST_SECTOR_ADDRESS_FOR_TX + page_offset;
    rx_page_addr = FLASH_LAST_SECTOR_ADDRESS_FOR_RX + page_offset;
}

// read and save page data
void readFlashData () {
    updatePageAddress();
    int* pointer_flash_value = (int *)rx_page_addr;
    for (auto pos = PAGE_STATUS_POS; pos < buf_num_data_used; ++pos, ++pointer_flash_value) {
        buf_rx[pos] = *pointer_flash_value;
    }
}

void readFlashSectorData () {
    int page_addr = 0;
    int* pointer_flash_value = nullptr;
    int value_arr[FLASH_NUM_PAGES_PER_SECTOR * buf_num_data_used] = {0};

    for (auto page = 0u; page < FLASH_NUM_PAGES_PER_SECTOR; ++page) {
        // page_addr = XIP_BASE +  FLASH_TARGET_LAST_SECTOR_IN_FLASH + (page * FLASH_PAGE_SIZE);
        page_addr = FLASH_LAST_SECTOR_ADDRESS_FOR_RX + (page * FLASH_PAGE_SIZE);
        pointer_flash_value = (int*)page_addr;

        for (auto pos = 0; pos < buf_num_data_used; ++pos, ++pointer_flash_value) {
            value_arr[(page * buf_num_data_used) + pos] = *pointer_flash_value;
        }
    }

    uint8_t counter = 0;
    for (auto& i : value_arr) {
        Serial.print(i);
        Serial.print(" ");

        ++counter;
        if (counter == buf_num_data_used) {
            Serial.println();
            counter = 0;
        }
    }

}

void updateFlashData () {
    
    updateCurrentPage();
    
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(tx_page_addr, (uint8_t *)buf_tx, FLASH_PAGE_SIZE);
    restore_interrupts (ints);
   
}

void eraseFlashData () {
    flash_range_erase(FLASH_LAST_SECTOR_ADDRESS_FOR_TX, FLASH_SECTOR_SIZE);
}

void resetFlashBuffer () {
    for (auto& val : buf_tx)
        val = PAGE_STATUS_EMPTY;
    for (auto& val : buf_rx)
        val = PAGE_STATUS_EMPTY;
}

void updateFlashBuffer () {
    static int max_val = 0;
    
    // resetFlashBuffer();
    buf_tx[PAGE_STATUS_POS] = PAGE_STATUS_USED;
    for (auto pos = 1; pos < buf_num_data_used; ++pos) {
        buf_tx[pos] =  max_val++;
    }
}

void updateFlashBuffer (uint8_t val) {
    buf_tx[PAGE_STATUS_POS] == PAGE_STATUS_EMPTY;
    for (auto pos = 1; pos < buf_num_of_data_var; ++pos) {
        buf_tx[pos] =  val + (current_page * buf_num_data_used + pos);
        buf_tx[pos] = PAGE_STATUS_EMPTY ^ (1u << (val * pos));
    }
}

void findCurrentPage () {
    
    readFlashSectorData();

    int page_satus_addr;
    Serial.println("--------------------------------------------");
    for (auto page = 0; page < FLASH_NUM_PAGES_PER_SECTOR; ++page) {
        // page_satus_addr = XIP_BASE +  FLASH_TARGET_LAST_SECTOR_IN_FLASH + (page * FLASH_PAGE_SIZE);
        page_satus_addr = FLASH_LAST_SECTOR_ADDRESS_FOR_RX + (page * FLASH_PAGE_SIZE);
        bool page_empty = ((*(int *)page_satus_addr) == (int)PAGE_STATUS_EMPTY)? true: false;

        Serial.print("page ");
        if (page < 10) Serial.print(" ");    // to align all prints 
        Serial.print(page);
        Serial.print("  -  ");
        Serial.println(page_empty? "Empty": "In use");

        // we found our first empty page, the page before is the current page
        if (page_empty){
            updateCurrentPage(page - 1);                    
            break;
        }
    }
    Serial.println("--------------------------------------------");

}


void printFlashBuffer () {
    Serial.print("Current page = ");
    Serial.println(current_page);
    Serial.println("Index    tx     rx");
    for (auto i = 0; i < buf_num_data_used; ++i) {
        Serial.print(i);
        Serial.print(".       ");
        Serial.print(buf_tx[i]);
        Serial.print("  -  ");
        Serial.print(buf_rx[i]);
        Serial.println(buf_tx[i] == buf_rx[i]? "   - OK": "   - Fault"); // check saved data is ok
    }
    Serial.println();
}

// ----------------------------------------------------------------



