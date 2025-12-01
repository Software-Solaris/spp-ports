# SPP Ports

This folder contains the hardware and RTOS bindings that make the Solaris Packet Protocol (SPP) runnable on real targets. Each port implements the abstract APIs defined in `external/spp/hal` and `external/spp/osal`, keeping the core protocol untouched while adapting it to specific MCUs and kernels.

## Directory layout
- `hal/`: hardware backends. The `esp32/` example wires the generic SPI HAL (`SPP_HAL_SPI_*`) to the ESP-IDF driver, adds ESP-specific macros, and provides a `main.example` and simple tests to verify the integration.
- `osal/`: operating-system backends. Currently `freertos/` implements the OSAL primitives (tasks, semaphores, queues, mutexes) on top of FreeRTOS and includes lightweight tests.

Add new targets by copying one of these folders and providing your own implementation that satisfies the HAL/OSAL contracts.

## Usage hints
1. Start from a working `external/spp` build and include the HAL/OSAL headers from this directory in your firmware project.
2. Implement any missing hooks required by SPP (SPI init, task spawning, synchronization). Use the ESP32/FreeRTOS examples as reference for required function signatures.
3. Rebuild the Doxygen docs in `external/spp/docs` if you need updated API references for porting work (`doxygen external/spp/Doxyfile`).

With these ports, SPP can be reused across multiple Solaris projects simply by selecting the right HAL/OSAL backend for the hardware in use.
