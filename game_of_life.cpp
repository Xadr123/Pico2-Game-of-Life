#include <stdio.h>
#include <cstdint>
#include <random>
#include <ctime>
#include <vector>
#include <malloc.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/uart.h"

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5

constexpr uint16_t Columns{100};
constexpr uint16_t Rows{20};

// Grid for cells (initialized based on settings values)
std::vector<std::vector<uint8_t>> Grid{Rows, std::vector<uint8_t>(Columns, 0)};

// Stores the new grid values.
std::vector<std::vector<uint8_t>> NewGrid{Rows, std::vector<uint8_t>(Columns, 0)};

// Sets up a random grid of alive/dead cells.
void initGrid(std::vector<std::vector<uint8_t>> &grid)
{
    for (int r = 0; r < Rows; ++r)
    {
        for (int c = 0; c < Columns; ++c)
        {
            grid[r][c] = ((std::rand() % 100) < 10) ? 1 : 0;
        }
    }
}

// Returns the number of neighboring ALIVE cells
uint8_t getNeighborCount(const std::vector<std::vector<uint8_t>> &grid, int row, int column)
{
    uint8_t count = 0;

    // Check all neighbors
    for (int r = -1; r <= 1; ++r)
    {
        for (int c = -1; c <= 1; ++c)
        {
            // Dont check self
            if (r == 0 && c == 0)
            {
                continue;
            }
            else
            {
                // Rows/Colums wrap (toroidal)
                int destRow = (row + r + Rows) % Rows;
                int destColumn = (column + c + Columns) % Columns;

                if (grid[destRow][destColumn])
                {
                    ++count;
                }
            }
        }
    }
    return count;
}

// Check all cell neighbors and each cell accordingly
void updateGrid(std::vector<std::vector<uint8_t>> &grid, std::vector<std::vector<uint8_t>> &newGrid)
{
    for (int r = 0; r < Rows; ++r)
    {
        for (int c = 0; c < Columns; ++c)
        {
            int neighborCount = getNeighborCount(grid, r, c);

            if (grid[r][c])
            {
                // Rule 1 and 2: Death on Underpopulation (less than 2 neigbors)/Overpopulation (more than 3 neighbors)
                if (neighborCount < 2 || neighborCount > 3)
                {
                    newGrid[r][c] = false;
                }
                else // Survive otherwise
                {
                    newGrid[r][c] = true;
                }
            }
            else
            {
                // Rule 3: Reproduce if 3 neighbors
                if (neighborCount == 3)
                {
                    newGrid[r][c] = true;
                }
                else // Death otherwise
                {
                    newGrid[r][c] = false;
                }
            }
        }
    }
    grid = newGrid;
}

void printGrid(std::vector<std::vector<uint8_t>> &grid)
{
    for (int r = 0; r < Rows; ++r)
    {
        for (int c = 0; c < Columns; ++c)
        {
            const char *cell{(Grid[r][c] == 0 ? "." : "X")};
            printf(cell);
        }
        printf("\n");
    }
}

uint32_t getTotalHeapMemory()
{
    extern char __StackLimit, __bss_end__;

    return &__StackLimit - &__bss_end__;
}

uint32_t getFreeHeapMemory()
{
    struct mallinfo m = mallinfo();

    return getTotalHeapMemory() - m.uordblks;
}

bool initPico()
{
    bool result{true};
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init())
    {
        printf("Wi-Fi init failed\n");
        result = false;
    }

    // Example to turn on the Pico W LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    return result;
}

int main()
{
    if (initPico())
    {
        initGrid(Grid);
        sleep_ms(1000);
        while (true)
        {
            printGrid(Grid);
            printf("------------------------------------------------------------------------------------------------------\n");
            // printf("Total: %d\n", getTotalHeapMemory());
            // printf("Free: %d\n", getFreeHeapMemory());
            // printf("Used: %d\n", getTotalHeapMemory() - getFreeHeapMemory());
            updateGrid(Grid, NewGrid);
            sleep_ms(500);
        }
    }
}
